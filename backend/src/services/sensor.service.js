import pool from '../config/database.js';
import logger from '../config/logger.js';

/**
 * Identifica sensor e elemento pelo MAC do node
 */
export async function identifySensorByMac(nodeMac, variable) {
  try {
    const query = `
      SELECT 
        s.sensor_id,
        s.elemento_id,
        s.tipo,
        s.ajuste_offset,
        e.nome as elemento_nome,
        e.parametros as elemento_parametros
      FROM aguada.sensores s
      JOIN aguada.elementos e ON s.elemento_id = e.elemento_id
      WHERE s.node_mac = $1 
        AND s.variavel = $2
        AND s.status = 'ativo'
      LIMIT 1
    `;
    
    const result = await pool.query(query, [nodeMac, variable]);
    
    if (result.rows.length === 0) {
      logger.warn(`Sensor não encontrado: MAC=${nodeMac}, variable=${variable}`);
      return null;
    }
    
    return result.rows[0];
  } catch (error) {
    logger.error('Erro ao identificar sensor:', error);
    throw error;
  }
}

/**
 * Busca sensor por ID
 */
export async function getSensorById(sensorId) {
  try {
    const query = `
      SELECT 
        s.*,
        e.nome as elemento_nome,
        e.tipo as elemento_tipo,
        e.parametros as elemento_parametros
      FROM aguada.sensores s
      JOIN aguada.elementos e ON s.elemento_id = e.elemento_id
      WHERE s.sensor_id = $1
    `;
    
    const result = await pool.query(query, [sensorId]);
    return result.rows.length > 0 ? result.rows[0] : null;
  } catch (error) {
    logger.error('Erro ao buscar sensor:', error);
    throw error;
  }
}

/**
 * Atualiza offset de calibração do sensor
 */
export async function updateSensorOffset(sensorId, offset) {
  try {
    const query = `
      UPDATE aguada.sensores
      SET ajuste_offset = $1, ultima_calibracao = NOW()
      WHERE sensor_id = $2
      RETURNING *
    `;
    
    const result = await pool.query(query, [offset, sensorId]);
    return result.rows[0];
  } catch (error) {
    logger.error('Erro ao atualizar offset do sensor:', error);
    throw error;
  }
}

export default {
  identifySensorByMac,
  getSensorById,
  updateSensorOffset,
};
