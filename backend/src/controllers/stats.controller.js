import { pool } from '../config/database.js';
import logger from '../config/logger.js';

/**
 * GET /api/stats/daily
 * Get daily statistics
 */
export async function getDailyStats(req, res) {
  try {
    const { date = new Date().toISOString().split('T')[0] } = req.query;

    const result = await pool.query(`
      SELECT 
        s.sensor_id,
        s.elemento_id,
        e.nome as elemento_nome,
        COUNT(*) as total_readings,
        MIN(l.valor) as min_value,
        MAX(l.valor) as max_value,
        AVG(l.valor) as avg_value,
        STDDEV(l.valor) as stddev_value
      FROM leituras_raw l
      JOIN sensores s ON l.sensor_id = s.sensor_id
      JOIN elementos e ON s.elemento_id = e.elemento_id
      WHERE DATE(l.datetime) = $1
      GROUP BY s.sensor_id, s.elemento_id, e.nome
      ORDER BY s.sensor_id
    `, [date]);

    res.status(200).json({
      success: true,
      date,
      count: result.rows.length,
      data: result.rows
    });
  } catch (error) {
    logger.error('Error fetching daily stats:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar estatísticas diárias'
    });
  }
}

/**
 * GET /api/stats/consumption
 * Get consumption statistics
 */
export async function getConsumptionStats(req, res) {
  try {
    const { 
      sensor_id,
      period = '7d', // 24h, 7d, 30d
      group_by = 'hour' // hour, day, week
    } = req.query;

    let interval;
    let dateFormat;

    switch (period) {
      case '24h':
        interval = '24 hours';
        dateFormat = group_by === 'hour' ? 'YYYY-MM-DD HH24:00:00' : 'YYYY-MM-DD';
        break;
      case '7d':
        interval = '7 days';
        dateFormat = 'YYYY-MM-DD';
        break;
      case '30d':
        interval = '30 days';
        dateFormat = 'YYYY-MM-DD';
        break;
      default:
        interval = '7 days';
        dateFormat = 'YYYY-MM-DD';
    }

    let query = `
      SELECT 
        TO_CHAR(datetime, '${dateFormat}') as period,
        sensor_id,
        COUNT(*) as readings_count,
        AVG(valor) as avg_value,
        MIN(valor) as min_value,
        MAX(valor) as max_value
      FROM leituras_raw
      WHERE datetime > NOW() - INTERVAL '${interval}'
    `;

    const params = [];
    if (sensor_id) {
      query += ` AND sensor_id = $1`;
      params.push(sensor_id);
    }

    query += `
      GROUP BY period, sensor_id
      ORDER BY period DESC, sensor_id
    `;

    const result = await pool.query(query, params);

    res.status(200).json({
      success: true,
      period,
      group_by,
      count: result.rows.length,
      data: result.rows
    });
  } catch (error) {
    logger.error('Error fetching consumption stats:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar estatísticas de consumo'
    });
  }
}

/**
 * GET /api/stats/sensors
 * Get sensors statistics summary
 */
export async function getSensorsStats(req, res) {
  try {
    const result = await pool.query(`
      SELECT 
        s.sensor_id,
        s.elemento_id,
        e.nome as elemento_nome,
        e.tipo as elemento_tipo,
        COUNT(l.leitura_id) as total_readings,
        MAX(l.datetime) as last_reading,
        MIN(l.datetime) as first_reading,
        CASE 
          WHEN MAX(l.datetime) > NOW() - INTERVAL '5 minutes' THEN 'online'
          WHEN MAX(l.datetime) > NOW() - INTERVAL '1 hour' THEN 'warning'
          ELSE 'offline'
        END as status
      FROM sensores s
      LEFT JOIN elementos e ON s.elemento_id = e.elemento_id
      LEFT JOIN leituras_raw l ON s.sensor_id = l.sensor_id
      GROUP BY s.sensor_id, s.elemento_id, e.nome, e.tipo
      ORDER BY s.sensor_id
    `);

    const summary = {
      total: result.rows.length,
      online: result.rows.filter(r => r.status === 'online').length,
      warning: result.rows.filter(r => r.status === 'warning').length,
      offline: result.rows.filter(r => r.status === 'offline').length,
      total_readings: result.rows.reduce((sum, r) => sum + parseInt(r.total_readings || 0), 0)
    };

    res.status(200).json({
      success: true,
      summary,
      data: result.rows
    });
  } catch (error) {
    logger.error('Error fetching sensors stats:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar estatísticas dos sensores'
    });
  }
}

/**
 * GET /api/stats/events
 * Get events statistics
 */
export async function getEventsStats(req, res) {
  try {
    const { period = '7d' } = req.query;

    let interval;
    switch (period) {
      case '24h': interval = '24 hours'; break;
      case '7d': interval = '7 days'; break;
      case '30d': interval = '30 days'; break;
      default: interval = '7 days';
    }

    const result = await pool.query(`
      SELECT 
        tipo_evento,
        COUNT(*) as count,
        DATE_TRUNC('day', datetime) as day
      FROM eventos
      WHERE datetime > NOW() - INTERVAL '${interval}'
      GROUP BY tipo_evento, day
      ORDER BY day DESC, tipo_evento
    `);

    const summary = result.rows.reduce((acc, row) => {
      if (!acc[row.tipo_evento]) {
        acc[row.tipo_evento] = 0;
      }
      acc[row.tipo_evento] += parseInt(row.count);
      return acc;
    }, {});

    res.status(200).json({
      success: true,
      period,
      summary,
      data: result.rows
    });
  } catch (error) {
    logger.error('Error fetching events stats:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar estatísticas de eventos'
    });
  }
}

export default {
  getDailyStats,
  getConsumptionStats,
  getSensorsStats,
  getEventsStats
};
