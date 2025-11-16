import readingService from './reading.service.js';
import volumeService from './volume.service.js';
import eventService from './event.service.js';
import logger from '../config/logger.js';

const DEADBAND_CM = parseFloat(process.env.DEADBAND_CM || '2.0');
const WINDOW_SIZE = parseInt(process.env.WINDOW_SIZE || '11');
const STABILITY_STDDEV = parseFloat(process.env.STABILITY_STDDEV || '0.5');

/**
 * Processador de compressão com algoritmo de deadband
 */
export async function processCompression(sensor, valor, elementoParametros, datetime) {
  try {
    const { sensor_id, elemento_id, variavel, ajuste_offset } = sensor;
    
    // Aplicar offset de calibração
    const valorAjustado = valor + (ajuste_offset || 0);
    
    // Calcular volume se for sensor de nível
    let volumeData = { volume_m3: null, percentual: null };
    if (variavel === 'nivel_cm' && elementoParametros) {
      volumeData = volumeService.calculateVolume(valorAjustado, elementoParametros);
    }
    
    // Buscar última leitura processada
    const lastProcessed = await readingService.getLastProcessedReading(elemento_id, variavel);
    
    if (!lastProcessed) {
      // Primeira leitura - inserir nova
      await readingService.insertProcessedReading({
        elemento_id,
        variavel,
        valor: valorAjustado,
        unidade: 'cm',
        volume_m3: volumeData.volume_m3,
        percentual: volumeData.percentual,
        criterio: 'primeira_leitura',
        variacao: 0,
        data_inicio: datetime,
        data_fim: datetime,
        fonte: 'sistema',
        autor: 'compression_engine',
        meta: null,
      });
      
      logger.info('Primeira leitura processada inserida', { elemento_id, valor: valorAjustado });
      return;
    }
    
    // Calcular variação
    const delta = Math.abs(valorAjustado - lastProcessed.valor);
    
    // Se dentro do deadband, estender período
    if (delta <= DEADBAND_CM) {
      await readingService.extendProcessedReading(lastProcessed.proc_id, datetime);
      logger.debug('Leitura dentro do deadband - período estendido', {
        elemento_id,
        delta,
        deadband: DEADBAND_CM,
      });
      return;
    }
    
    // Verificar estabilidade (window)
    const recentReadings = await readingService.getRecentReadings(elemento_id, variavel, WINDOW_SIZE);
    const stddev = volumeService.calculateStdDev(recentReadings);
    
    // Se instável, aguardar mais leituras
    if (stddev > STABILITY_STDDEV && recentReadings.length < WINDOW_SIZE) {
      await readingService.extendProcessedReading(lastProcessed.proc_id, datetime);
      logger.debug('Aguardando estabilização', { elemento_id, stddev });
      return;
    }
    
    // Mudança significativa - inserir nova leitura processada
    await readingService.insertProcessedReading({
      elemento_id,
      variavel,
      valor: valorAjustado,
      unidade: 'cm',
      volume_m3: volumeData.volume_m3,
      percentual: volumeData.percentual,
      criterio: `delta=${delta.toFixed(2)}cm stddev=${stddev.toFixed(2)}`,
      variacao: delta,
      data_inicio: datetime,
      data_fim: datetime,
      fonte: 'sistema',
      autor: 'compression_engine',
      meta: { stddev, window_size: recentReadings.length },
    });
    
    logger.info('Nova leitura processada (mudança significativa)', {
      elemento_id,
      delta,
      volume_m3: volumeData.volume_m3,
    });
    
    // Detectar eventos se houver volume calculado
    if (volumeData.volume_m3 !== null && lastProcessed.volume_m3 !== null) {
      await detectEvents(elemento_id, lastProcessed.volume_m3, volumeData.volume_m3, volumeData.percentual, datetime);
    }
    
  } catch (error) {
    logger.error('Erro no processamento de compressão:', error);
    throw error;
  }
}

/**
 * Detecta eventos após mudança de volume
 */
async function detectEvents(elementoId, volumeAnteriorM3, volumeAtualM3, percentualAtual, datetime) {
  try {
    // Detectar abastecimento
    await eventService.detectSupplyEvent(elementoId, volumeAnteriorM3, volumeAtualM3, datetime);
    
    // Detectar vazamento
    await eventService.detectLeakEvent(elementoId);
    
    // Detectar nível crítico (CAV)
    await eventService.detectCriticalLevel(elementoId, percentualAtual, datetime);
    
  } catch (error) {
    logger.error('Erro na detecção de eventos:', error);
  }
}

export default {
  processCompression,
};
