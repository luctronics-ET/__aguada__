import pool from "../config/database.js";
import logger from "../config/logger.js";

/**
 * Verifica se a tabela aguada.elementos existe
 */
async function checkElementosTableExists() {
  try {
    const result = await pool.query(`
      SELECT EXISTS (
        SELECT FROM information_schema.tables 
        WHERE table_schema = 'aguada' 
        AND table_name = 'elementos'
      );
    `);
    return result.rows[0].exists;
  } catch (error) {
    return false;
  }
}

/**
 * Identifica sensor e elemento pelo MAC do node
 */
export async function identifySensorByMac(nodeMac, variable) {
  try {
    const hasElementos = await checkElementosTableExists();

    let query;
    if (hasElementos) {
      query = `
        SELECT 
          s.sensor_id,
          s.elemento_id,
          s.tipo,
          s.ajuste_offset,
          s.variavel,
          e.nome as elemento_nome,
          e.parametros as elemento_parametros
        FROM aguada.sensores s
        LEFT JOIN aguada.elementos e ON s.elemento_id = e.elemento_id
        WHERE s.node_mac = $1 
          AND s.variavel = $2
          AND s.status = 'ativo'
        LIMIT 1
      `;
    } else {
      query = `
        SELECT 
          s.sensor_id,
          s.elemento_id,
          s.tipo,
          s.ajuste_offset,
          s.variavel,
          NULL as elemento_nome,
          NULL as elemento_parametros
        FROM aguada.sensores s
        WHERE s.node_mac = $1 
          AND s.variavel = $2
          AND s.status = 'ativo'
        LIMIT 1
      `;
    }

    const result = await pool.query(query, [nodeMac, variable]);

    if (result.rows.length === 0) {
      logger.warn(
        `Sensor não encontrado: MAC=${nodeMac}, variable=${variable}`
      );
      return null;
    }

    return result.rows[0];
  } catch (error) {
    logger.error("Erro ao identificar sensor:", error);
    throw error;
  }
}

/**
 * Identifica sensor apenas pelo MAC (formato AGUADA-1 - sempre distance_mm)
 * Retorna o primeiro sensor ativo com variavel 'distance_cm' ou 'nivel_cm'
 * Usa LEFT JOIN para funcionar mesmo sem tabela elementos
 */
export async function identifySensorByMacOnly(nodeMac) {
  try {
    // Primeiro, tentar query com elementos (se existir)
    try {
      const queryWithElementos = `
        SELECT 
          s.sensor_id,
          s.elemento_id,
          s.tipo,
          s.ajuste_offset,
          s.variavel,
          e.nome as elemento_nome,
          e.parametros as elemento_parametros
        FROM aguada.sensores s
        LEFT JOIN aguada.elementos e ON s.elemento_id = e.elemento_id
        WHERE s.node_mac = $1 
          AND s.status = 'ativo'
          AND (s.variavel = 'distance_cm' OR s.variavel = 'nivel_cm')
        ORDER BY s.variavel DESC
        LIMIT 1
      `;

      const result = await pool.query(queryWithElementos, [nodeMac]);

      if (result.rows.length > 0) {
        return result.rows[0];
      }
    } catch (elementosError) {
      // Tabela elementos pode não existir, tentar query simplificada
      logger.warn("Tabela elementos não disponível, usando query simplificada");
    }

    // Fallback: query sem elementos
    const querySimple = `
      SELECT 
        s.sensor_id,
        s.elemento_id,
        s.tipo,
        s.ajuste_offset,
        s.variavel,
        NULL as elemento_nome,
        NULL as elemento_parametros
      FROM aguada.sensores s
      WHERE s.node_mac = $1 
        AND s.status = 'ativo'
        AND (s.variavel = 'distance_cm' OR s.variavel = 'nivel_cm')
      ORDER BY s.variavel DESC
      LIMIT 1
    `;

    const result = await pool.query(querySimple, [nodeMac]);

    if (result.rows.length === 0) {
      logger.warn(`Sensor não encontrado: MAC=${nodeMac} (formato AGUADA-1)`);
      return null;
    }

    return result.rows[0];
  } catch (error) {
    logger.error("Erro ao identificar sensor por MAC:", error);
    throw error;
  }
}

/**
 * Busca sensor por ID
 */
export async function getSensorById(sensorId) {
  try {
    const hasElementos = await checkElementosTableExists();

    let query;
    if (hasElementos) {
      query = `
        SELECT 
          s.*,
          e.nome as elemento_nome,
          e.tipo as elemento_tipo,
          e.parametros as elemento_parametros
        FROM aguada.sensores s
        LEFT JOIN aguada.elementos e ON s.elemento_id = e.elemento_id
        WHERE s.sensor_id = $1
      `;
    } else {
      query = `
        SELECT 
          s.*,
          NULL as elemento_nome,
          NULL as elemento_tipo,
          NULL as elemento_parametros
        FROM aguada.sensores s
        WHERE s.sensor_id = $1
      `;
    }

    const result = await pool.query(query, [sensorId]);
    return result.rows.length > 0 ? result.rows[0] : null;
  } catch (error) {
    logger.error("Erro ao buscar sensor:", error);
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
    logger.error("Erro ao atualizar offset do sensor:", error);
    throw error;
  }
}

export default {
  identifySensorByMac,
  identifySensorByMacOnly,
  getSensorById,
  updateSensorOffset,
};
