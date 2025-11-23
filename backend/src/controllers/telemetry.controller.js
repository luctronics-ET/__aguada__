import { validateTelemetry, validateIndividualTelemetry, validateManualReading, validateCalibration } from '../schemas/telemetry.schema.js';
import sensorService from '../services/sensor.service.js';
import readingService from '../services/reading.service.js';
import compressionService from '../services/compression.service.js';
import cacheService from '../services/cache.service.js';
import queueService from '../services/queue.service.js';
import duplicateService from '../services/duplicate.service.js';
import metricsService from '../services/metrics.service.js';
import logger from '../config/logger.js';
import { broadcastReading } from '../websocket/wsHandler.js';

/**
 * POST /api/telemetry
 * Endpoint para receber telemetria dos nodes ESP32
 * Suporta dois formatos:
 * 1. Individual: {"mac":"...","type":"distance_cm","value":24480,"battery":5000,"uptime":3,"rssi":-50}
 * 2. Agregado: {"node_mac":"...","datetime":"...","data":[...],"meta":{...}}
 */
export async function receiveTelemetry(req, res) {
  const startTime = Date.now();
  
  try {
    // Registrar recebimento de telemetria
    metricsService.recordTelemetryReceived();
    
    // Detectar formato baseado na presença do campo 'mac' vs 'node_mac'
    const isIndividualFormat = 'mac' in req.body && 'type' in req.body;
    
    let result;
    if (isIndividualFormat) {
      result = await receiveIndividualTelemetry(req, res);
    } else {
      result = await receiveAggregatedTelemetry(req, res);
    }
    
    // Registrar latência
    const latency = Date.now() - startTime;
    metricsService.recordLatency(latency);
    metricsService.recordEndpointRequest('POST', '/api/telemetry', res.statusCode || 200, latency);
    
    // Registrar processamento bem-sucedido
    if (res.statusCode < 400) {
      metricsService.recordTelemetryProcessed();
    } else {
      metricsService.recordTelemetryFailed();
    }
    
    return result;
  } catch (error) {
    // Registrar erro
    const latency = Date.now() - startTime;
    metricsService.recordLatency(latency);
    metricsService.recordEndpointRequest('POST', '/api/telemetry', 500, latency);
    metricsService.recordTelemetryFailed();
    metricsService.recordError('telemetry_error', error.message);
    
    logger.error('Erro ao receber telemetria:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro interno do servidor',
    });
  }
}

/**
 * Processa telemetria individual (formato firmware)
 */
async function receiveIndividualTelemetry(req, res) {
  try {
    // Validar payload
    const validation = validateIndividualTelemetry(req.body);
    
    if (!validation.success) {
      logger.warn('Telemetria individual inválida', { errors: validation.error.errors });
      return res.status(400).json({
        success: false,
        error: 'Validação falhou',
        details: validation.error.errors,
      });
    }
    
    const { mac, type, value, battery, rssi, uptime } = validation.data;
    
    // Identificar sensor pelo MAC + tipo de variável
    const sensor = await sensorService.identifySensorByMac(mac, type);
    
    if (!sensor) {
      logger.warn(`Sensor desconhecido: MAC=${mac}, type=${type}`);
      return res.status(404).json({
        success: false,
        error: 'Sensor não registrado',
        mac,
        type,
      });
    }
    
    // Converter valor conforme tipo
    let valorReal = value;
    let unidade = '';
    
    if (type === 'distance_cm') {
      // distance_cm vem multiplicado por 10 (ex: 2448 = 244.8 cm)
      valorReal = value / 100.0;
      unidade = 'cm';
    } else {
      // valve_in, valve_out, sound_in são estados (0 ou 1)
      valorReal = value;
      unidade = 'boolean';
    }
    
    const datetime = new Date();
    
    // Verificar duplicata antes de inserir
    const isDup = await duplicateService.isDuplicate(sensor.sensor_id, datetime, valorReal);
    if (isDup) {
      logger.warn('Leitura duplicada ignorada', {
        sensor_id: sensor.sensor_id,
        type,
        valor: valorReal,
      });
      return res.status(200).json({
        success: true,
        message: 'Leitura duplicada ignorada',
        sensor_id: sensor.sensor_id,
        duplicate: true,
      });
    }
    
    // Inserir leitura bruta
    await readingService.insertRawReading({
      sensor_id: sensor.sensor_id,
      elemento_id: sensor.elemento_id,
      variavel: type,
      valor: valorReal,
      unidade,
      meta: {
        battery_mv: battery,
        rssi_dbm: rssi,
        uptime_sec: uptime,
        node_mac: mac,
        raw_value: value,
      },
      fonte: 'sensor',
      autor: mac,
      modo: 'automatica',
      observacao: null,
      datetime,
    });
    
    // Adicionar à fila para processamento assíncrono (não bloqueia API)
    queueService.enqueueReading({
      sensor,
      valorReal,
      elementoParametros: sensor.elemento_parametros,
      datetime,
      type,
    }).catch(err => {
      logger.error('Erro ao adicionar leitura à fila:', err);
      // Fallback: processar diretamente se a fila falhar
      if (type === 'distance_cm') {
        compressionService.processCompression(
          sensor,
          valorReal,
          sensor.elemento_parametros,
          datetime
        ).catch(compErr => {
          logger.error('Erro no processamento de fallback:', compErr);
        });
      }
    });
    
    // Invalidar cache de leituras (assíncrono, não bloqueia)
    cacheService.invalidateReadings().catch(err => {
      logger.warn('Erro ao invalidar cache:', err);
    });
    
    logger.info('Telemetria individual recebida', {
      mac,
      type,
      value: valorReal,
      sensor_id: sensor.sensor_id,
    });
    
    return res.status(200).json({
      success: true,
      message: 'Telemetria recebida com sucesso',
      sensor_id: sensor.sensor_id,
      type,
      value: valorReal,
    });
    
  } catch (error) {
    logger.error('Erro ao processar telemetria individual:', error);
    throw error;
  }
}

/**
 * Processa telemetria agregada (formato legado)
 */
async function receiveAggregatedTelemetry(req, res) {
  try {
    // Validar payload
    const validation = validateTelemetry(req.body);
    
    if (!validation.success) {
      logger.warn('Telemetria inválida', { errors: validation.error.errors });
      return res.status(400).json({
        success: false,
        error: 'Validação falhou',
        details: validation.error.errors,
      });
    }
    
    const { node_mac, datetime, data, meta } = validation.data;
    
    const processedReadings = [];
    
    // Processar cada leitura do payload
    for (const reading of data) {
      const { label, value, unit } = reading;
      
      // Identificar sensor pelo MAC + variável
      const sensor = await sensorService.identifySensorByMac(node_mac, label);
      
      if (!sensor) {
        logger.warn(`Sensor desconhecido: MAC=${node_mac}, label=${label}`);
        continue; // Pular leitura desconhecida
      }
      
      // Inserir leitura bruta
      await readingService.insertRawReading({
        sensor_id: sensor.sensor_id,
        elemento_id: sensor.elemento_id,
        variavel: label,
        valor: value,
        unidade: unit || 'cm',
        meta: {
          ...meta,
          node_mac,
        },
        fonte: 'sensor',
        autor: node_mac,
        modo: 'automatica',
        observacao: null,
        datetime: new Date(datetime),
      });
      
      // Adicionar à fila para processamento assíncrono
      queueService.enqueueReading({
        sensor,
        valorReal: value,
        elementoParametros: sensor.elemento_parametros,
        datetime: new Date(datetime),
        type: label,
      }).catch(err => {
        logger.error('Erro ao adicionar leitura à fila:', err);
        // Fallback: processar diretamente
        compressionService.processCompression(
          sensor,
          value,
          sensor.elemento_parametros,
          new Date(datetime)
        ).catch(compErr => {
          logger.error('Erro no processamento de fallback:', compErr);
        });
      });
      
      processedReadings.push({
        sensor_id: sensor.sensor_id,
        elemento_id: sensor.elemento_id,
        valor: value,
      });

      // Broadcast reading via WebSocket
      broadcastReading({
        sensor_id: sensor.sensor_id,
        mac: node_mac,
        label: reading.label,
        value: value,
        datetime: datetime
      });
    }
    
    // Invalidar cache de leituras
    cacheService.invalidateReadings().catch(err => {
      logger.warn('Erro ao invalidar cache:', err);
    });
    
    logger.info('Telemetria recebida', {
      node_mac,
      readings: processedReadings.length,
    });
    
    return res.status(200).json({
      success: true,
      message: 'Telemetria recebida com sucesso',
      processed: processedReadings.length,
    });
    
  } catch (error) {
    logger.error('Erro ao receber telemetria:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro interno do servidor',
    });
  }
}

/**
 * POST /api/manual-reading
 * Endpoint para leituras manuais feitas por operadores
 */
export async function receiveManualReading(req, res) {
  try {
    // Validar payload
    const validation = validateManualReading(req.body);
    
    if (!validation.success) {
      logger.warn('Leitura manual inválida', { errors: validation.error.errors });
      return res.status(400).json({
        success: false,
        error: 'Validação falhou',
        details: validation.error.errors,
      });
    }
    
    const { sensor_id, value, variable, datetime, usuario, observacao } = validation.data;
    
    // Buscar sensor
    const sensor = await sensorService.getSensorById(sensor_id);
    
    if (!sensor) {
      logger.warn(`Sensor não encontrado: ${sensor_id}`);
      return res.status(404).json({
        success: false,
        error: 'Sensor não encontrado',
      });
    }
    
    // Se recebeu nivel_cm mas o sensor usa distance_cm, converter
    let variavelFinal = variable;
    let valorFinal = value;
    
    if (variable === 'nivel_cm' && sensor.variavel === 'distance_cm') {
      // Converter nível para distância
      // nivel_cm = altura_cm - distancia_cm + offset
      // distancia_cm = altura_cm - nivel_cm + offset
      // Precisamos dos parâmetros do elemento para calcular
      const elementoParametros = sensor.elemento_parametros || {};
      const altura_cm = elementoParametros.altura_cm || 400;
      const offset_cm = elementoParametros.offset_cm || elementoParametros.hsensor_cm || 20;
      valorFinal = altura_cm - value + offset_cm;
      variavelFinal = 'distance_cm';
      logger.info('Convertido nivel_cm para distance_cm', { nivel_cm: value, distance_cm: valorFinal });
    }
    
    // Inserir leitura bruta
    await readingService.insertRawReading({
      sensor_id: sensor.sensor_id,
      elemento_id: sensor.elemento_id,
      variavel: variavelFinal,
      valor: valorFinal,
      unidade: variable === 'nivel_cm' ? 'cm' : (variable === 'pressao_bar' ? 'bar' : 'lpm'),
      meta: {
        manual: true,
        volume_m3: observacao?.includes('Volume:') ? parseFloat(observacao.match(/Volume: ([\d.]+)/)?.[1] || 0) : null,
        original_variable: variable,
        original_value: value,
      },
      fonte: 'usuario',
      autor: usuario,
      modo: 'manual',
      observacao,
      datetime: datetime ? new Date(datetime) : new Date(),
    });
    
    logger.info('Leitura manual recebida', {
      sensor_id,
      usuario,
      valor: value,
    });
    
    return res.status(200).json({
      success: true,
      message: 'Leitura manual registrada com sucesso',
    });
    
  } catch (error) {
    logger.error('Erro ao receber leitura manual:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro interno do servidor',
    });
  }
}

/**
 * POST /api/calibration
 * Endpoint para registrar calibração de sensores
 */
export async function receiveCalibration(req, res) {
  try {
    // Validar payload
    const validation = validateCalibration(req.body);
    
    if (!validation.success) {
      logger.warn('Calibração inválida', { errors: validation.error.errors });
      return res.status(400).json({
        success: false,
        error: 'Validação falhou',
        details: validation.error.errors,
      });
    }
    
    const {
      sensor_id,
      valor_referencia,
      valor_sensor,
      responsavel_usuario_id,
      tipo,
      observacao,
    } = validation.data;
    
    // Buscar sensor
    const sensor = await sensorService.getSensorById(sensor_id);
    
    if (!sensor) {
      logger.warn(`Sensor não encontrado: ${sensor_id}`);
      return res.status(404).json({
        success: false,
        error: 'Sensor não encontrado',
      });
    }
    
    // Calcular ajuste (offset)
    const ajuste = valor_referencia - valor_sensor;
    
    // Registrar calibração
    const query = `
      INSERT INTO aguada.calibracoes (
        sensor_id, elemento_id, responsavel_usuario_id,
        valor_referencia, valor_sensor, ajuste_aplicado,
        tipo, observacao
      ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
      RETURNING calibracao_id
    `;
    
    const pool = (await import('../config/database.js')).default;
    const result = await pool.query(query, [
      sensor_id,
      sensor.elemento_id,
      responsavel_usuario_id,
      valor_referencia,
      valor_sensor,
      ajuste,
      tipo,
      observacao,
    ]);
    
    // Atualizar offset no sensor
    await sensorService.updateSensorOffset(sensor_id, ajuste);
    
    logger.info('Calibração registrada', {
      calibracao_id: result.rows[0].calibracao_id,
      sensor_id,
      ajuste,
    });
    
    return res.status(200).json({
      success: true,
      message: 'Calibração registrada com sucesso',
      calibracao_id: result.rows[0].calibracao_id,
      ajuste_aplicado: ajuste,
    });
    
  } catch (error) {
    logger.error('Erro ao registrar calibração:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro interno do servidor',
    });
  }
}

export default {
  receiveTelemetry,
  receiveManualReading,
  receiveCalibration,
};
