import { pool } from '../config/database.js';
import logger from '../config/logger.js';

/**
 * GET /api/sensors
 * Get all sensors
 */
export async function getAllSensors(req, res) {
  try {
    const result = await pool.query(`
      SELECT 
        s.sensor_id,
        s.elemento_id,
        s.tipo_sensor,
        s.localizacao,
        s.status,
        s.data_instalacao,
        s.ultima_calibracao,
        e.nome as elemento_nome,
        e.tipo as elemento_tipo
      FROM sensores s
      LEFT JOIN elementos e ON s.elemento_id = e.elemento_id
      ORDER BY s.sensor_id
    `);

    res.status(200).json({
      success: true,
      count: result.rows.length,
      data: result.rows
    });
  } catch (error) {
    logger.error('Error fetching sensors:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar sensores'
    });
  }
}

/**
 * GET /api/sensors/:sensor_id
 * Get sensor by ID
 */
export async function getSensorById(req, res) {
  try {
    const { sensor_id } = req.params;

    const result = await pool.query(`
      SELECT 
        s.*,
        e.nome as elemento_nome,
        e.tipo as elemento_tipo,
        e.parametros as elemento_parametros
      FROM sensores s
      LEFT JOIN elementos e ON s.elemento_id = e.elemento_id
      WHERE s.sensor_id = $1
    `, [sensor_id]);

    if (result.rows.length === 0) {
      return res.status(404).json({
        success: false,
        error: 'Sensor não encontrado'
      });
    }

    res.status(200).json({
      success: true,
      data: result.rows[0]
    });
  } catch (error) {
    logger.error('Error fetching sensor:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar sensor'
    });
  }
}

/**
 * PUT /api/sensors/:sensor_id
 * Update sensor configuration
 */
export async function updateSensor(req, res) {
  try {
    const { sensor_id } = req.params;
    const updates = req.body;

    // Build dynamic update query
    const allowedFields = ['tipo_sensor', 'localizacao', 'status', 'parametros', 'observacoes'];
    const setClause = [];
    const values = [];
    let paramCount = 1;

    for (const field of allowedFields) {
      if (updates[field] !== undefined) {
        setClause.push(`${field} = $${paramCount}`);
        values.push(updates[field]);
        paramCount++;
      }
    }

    if (setClause.length === 0) {
      return res.status(400).json({
        success: false,
        error: 'Nenhum campo válido para atualizar'
      });
    }

    values.push(sensor_id);
    const query = `
      UPDATE sensores 
      SET ${setClause.join(', ')}, 
          data_atualizacao = NOW()
      WHERE sensor_id = $${paramCount}
      RETURNING *
    `;

    const result = await pool.query(query, values);

    if (result.rows.length === 0) {
      return res.status(404).json({
        success: false,
        error: 'Sensor não encontrado'
      });
    }

    logger.info(`Sensor ${sensor_id} updated`, updates);

    res.status(200).json({
      success: true,
      message: 'Sensor atualizado com sucesso',
      data: result.rows[0]
    });
  } catch (error) {
    logger.error('Error updating sensor:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao atualizar sensor'
    });
  }
}

/**
 * GET /api/sensors/status
 * Get status of all sensors
 */
export async function getSensorsStatus(req, res) {
  try {
    const result = await pool.query(`
      SELECT 
        s.sensor_id,
        s.elemento_id,
        s.status,
        e.nome as elemento_nome,
        MAX(l.datetime) as ultima_leitura,
        CASE 
          WHEN MAX(l.datetime) > NOW() - INTERVAL '5 minutes' THEN 'online'
          WHEN MAX(l.datetime) > NOW() - INTERVAL '1 hour' THEN 'warning'
          ELSE 'offline'
        END as estado_conexao
      FROM sensores s
      LEFT JOIN elementos e ON s.elemento_id = e.elemento_id
      LEFT JOIN leituras_raw l ON s.sensor_id = l.sensor_id
      GROUP BY s.sensor_id, s.elemento_id, s.status, e.nome
      ORDER BY s.sensor_id
    `);

    const summary = {
      total: result.rows.length,
      online: result.rows.filter(r => r.estado_conexao === 'online').length,
      warning: result.rows.filter(r => r.estado_conexao === 'warning').length,
      offline: result.rows.filter(r => r.estado_conexao === 'offline').length
    };

    res.status(200).json({
      success: true,
      summary,
      data: result.rows
    });
  } catch (error) {
    logger.error('Error fetching sensors status:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar status dos sensores'
    });
  }
}

export default {
  getAllSensors,
  getSensorById,
  updateSensor,
  getSensorsStatus
};
