import logger from '../config/logger.js';
import pool from '../config/database.js';

/**
 * GET /api/readings/latest
 * Obter últimas leituras de todos os sensores (últimas 24h)
 */
export async function getLatestReadings(req, res) {
  try {
    const query = `
      SELECT 
        s.sensor_id,
        s.nome_sensor,
        s.mac_address,
        s.elemento_id,
        r.variavel,
        r.valor,
        r.unidade,
        r.datetime,
        r.fonte
      FROM aguada.sensores s
      LEFT JOIN LATERAL (
        SELECT variavel, valor, unidade, datetime, fonte
        FROM aguada.leituras_raw
        WHERE sensor_id = s.sensor_id
        ORDER BY datetime DESC
        LIMIT 1
      ) r ON true
      WHERE s.ativo = true
      ORDER BY s.elemento_id, s.sensor_id;
    `;

    const result = await pool.query(query);
    
    // Format response
    const readings = {};
    result.rows.forEach(row => {
      if (!readings[row.sensor_id]) {
        readings[row.sensor_id] = {
          sensor_id: row.sensor_id,
          nome_sensor: row.nome_sensor,
          mac_address: row.mac_address,
          elemento_id: row.elemento_id,
          variables: {}
        };
      }
      
      if (row.variavel) {
        readings[row.sensor_id].variables[row.variavel] = {
          valor: row.valor,
          unidade: row.unidade,
          datetime: row.datetime,
          fonte: row.fonte,
        };
      }
    });

    logger.info('Leituras mais recentes enviadas', { sensores: Object.keys(readings).length });

    return res.status(200).json({
      success: true,
      data: readings,
      timestamp: new Date().toISOString(),
    });

  } catch (error) {
    logger.error('Erro ao obter leituras recentes:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro ao obter leituras',
    });
  }
}

/**
 * GET /api/readings/daily-summary
 * Resumo diário de cada sensor (min, max, média)
 */
export async function getDailySummary(req, res) {
  try {
    const query = `
      SELECT 
        s.sensor_id,
        s.nome_sensor,
        r.variavel,
        DATE(r.datetime) as data,
        COUNT(*) as total_leituras,
        MIN(r.valor) as valor_minimo,
        MAX(r.valor) as valor_maximo,
        ROUND(AVG(r.valor)::numeric, 2) as valor_medio,
        r.unidade
      FROM aguada.sensores s
      JOIN aguada.leituras_raw r ON s.sensor_id = r.sensor_id
      WHERE DATE(r.datetime) = CURRENT_DATE
        AND s.ativo = true
      GROUP BY s.sensor_id, s.nome_sensor, r.variavel, DATE(r.datetime), r.unidade
      ORDER BY s.elemento_id, s.sensor_id, r.variavel;
    `;

    const result = await pool.query(query);

    logger.info('Resumo diário enviado', { linhas: result.rowCount });

    return res.status(200).json({
      success: true,
      data: result.rows,
      date: new Date().toISOString().split('T')[0],
    });

  } catch (error) {
    logger.error('Erro ao obter resumo diário:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro ao obter resumo diário',
    });
  }
}

/**
 * GET /api/readings/history/:sensor_id
 * Histórico de leituras de um sensor (últimos N dias)
 */
export async function getReadingHistory(req, res) {
  try {
    const { sensor_id } = req.params;
    const { days = 7, variavel } = req.query;

    let query = `
      SELECT 
        datetime,
        variavel,
        valor,
        unidade,
        fonte
      FROM aguada.leituras_raw
      WHERE sensor_id = $1
        AND datetime >= NOW() - INTERVAL '${parseInt(days)} days'
    `;

    const params = [sensor_id];

    if (variavel) {
      query += ` AND variavel = $2`;
      params.push(variavel);
    }

    query += ` ORDER BY datetime DESC LIMIT 10000;`;

    const result = await pool.query(query, params);

    logger.info('Histórico de leituras enviado', { 
      sensor_id, 
      linhas: result.rowCount 
    });

    return res.status(200).json({
      success: true,
      sensor_id,
      variavel: variavel || 'todas',
      days: parseInt(days),
      data: result.rows,
    });

  } catch (error) {
    logger.error('Erro ao obter histórico:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro ao obter histórico',
    });
  }
}

/**
 * GET /api/sensors/status
 * Status de todos os sensores (online/offline, last_reading)
 */
export async function getSensorsStatus(req, res) {
  try {
    const query = `
      SELECT 
        s.sensor_id,
        s.nome_sensor,
        s.mac_address,
        s.elemento_id,
        s.ativo,
        MAX(r.datetime) as ultima_leitura,
        CASE 
          WHEN MAX(r.datetime) >= NOW() - INTERVAL '5 minutes' THEN 'online'
          ELSE 'offline'
        END as status,
        ROUND(EXTRACT(EPOCH FROM (NOW() - MAX(r.datetime))) / 60, 0)::int as minutos_sem_dados
      FROM aguada.sensores s
      LEFT JOIN aguada.leituras_raw r ON s.sensor_id = r.sensor_id
      GROUP BY s.sensor_id, s.nome_sensor, s.mac_address, s.elemento_id, s.ativo
      ORDER BY s.elemento_id, s.sensor_id;
    `;

    const result = await pool.query(query);

    logger.info('Status dos sensores enviado', { sensores: result.rowCount });

    return res.status(200).json({
      success: true,
      data: result.rows,
      timestamp: new Date().toISOString(),
    });

  } catch (error) {
    logger.error('Erro ao obter status dos sensores:', error);
    return res.status(500).json({
      success: false,
      error: 'Erro ao obter status',
    });
  }
}

/**
 * GET /api/readings/raw
 * Obter leituras raw com paginação
 */
export async function getRawReadings(req, res) {
  try {
    const limit = parseInt(req.query.limit) || 500;
    const offset = parseInt(req.query.offset) || 0;
    const elemento_id = req.query.elemento_id;
    const variavel = req.query.variavel;

    let whereClause = 'WHERE 1=1';
    const params = [];
    let paramIndex = 1;

    if (elemento_id) {
      whereClause += ` AND r.elemento_id = $${paramIndex}`;
      params.push(elemento_id);
      paramIndex++;
    }

    if (variavel) {
      whereClause += ` AND r.variavel = $${paramIndex}`;
      params.push(variavel);
      paramIndex++;
    }

    const query = `
      SELECT 
        r.datetime,
        r.sensor_id,
        r.elemento_id,
        r.variavel,
        r.valor,
        r.unidade,
        r.meta,
        r.fonte
      FROM leituras_raw r
      ${whereClause}
      ORDER BY r.datetime DESC
      LIMIT $${paramIndex}
      OFFSET $${paramIndex + 1}
    `;

    params.push(limit, offset);

    const result = await pool.query(query, params);

    res.status(200).json({
      success: true,
      count: result.rows.length,
      limit,
      offset,
      data: result.rows
    });
  } catch (error) {
    logger.error('Erro ao obter leituras raw:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao obter leituras'
    });
  }
}

export default {
  getLatestReadings,
  getDailySummary,
  getReadingHistory,
  getSensorsStatus,
  getRawReadings,
};
