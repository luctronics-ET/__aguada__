import pool from '../config/database.js';
import logger from '../config/logger.js';

/**
 * Insere leitura bruta no banco
 */
export async function insertRawReading(data) {
  const {
    sensor_id,
    elemento_id,
    variavel,
    valor,
    unidade,
    meta,
    fonte,
    autor,
    modo,
    observacao,
    datetime,
  } = data;
  
  try {
    const query = `
      INSERT INTO aguada.leituras_raw (
        sensor_id, elemento_id, variavel, valor, unidade,
        meta, fonte, autor, modo, observacao, datetime
      ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
      RETURNING leitura_id
    `;
    
    const values = [
      sensor_id,
      elemento_id,
      variavel,
      valor,
      unidade || 'cm',
      meta ? JSON.stringify(meta) : null,
      fonte,
      autor,
      modo || 'automatica',
      observacao || null,
      datetime || new Date(),
    ];
    
    const result = await pool.query(query, values);
    logger.info('Leitura raw inserida', { 
      leitura_id: result.rows[0].leitura_id,
      sensor_id,
      valor,
    });
    
    return result.rows[0];
  } catch (error) {
    logger.error('Erro ao inserir leitura raw:', error);
    throw error;
  }
}

/**
 * Busca última leitura processada para um elemento
 */
export async function getLastProcessedReading(elementoId, variavel) {
  try {
    const query = `
      SELECT *
      FROM aguada.leituras_processadas
      WHERE elemento_id = $1 AND variavel = $2
      ORDER BY data_fim DESC
      LIMIT 1
    `;
    
    const result = await pool.query(query, [elementoId, variavel]);
    return result.rows.length > 0 ? result.rows[0] : null;
  } catch (error) {
    logger.error('Erro ao buscar última leitura processada:', error);
    throw error;
  }
}

/**
 * Insere leitura processada (compressão)
 */
export async function insertProcessedReading(data) {
  const {
    elemento_id,
    variavel,
    valor,
    unidade,
    volume_m3,
    percentual,
    criterio,
    variacao,
    data_inicio,
    data_fim,
    fonte,
    autor,
    meta,
  } = data;
  
  try {
    const query = `
      INSERT INTO aguada.leituras_processadas (
        elemento_id, variavel, valor, unidade, volume_m3, percentual,
        criterio, variacao, data_inicio, data_fim, fonte, autor, meta
      ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)
      RETURNING proc_id
    `;
    
    const values = [
      elemento_id,
      variavel,
      valor,
      unidade || 'cm',
      volume_m3,
      percentual,
      criterio,
      variacao,
      data_inicio,
      data_fim,
      fonte,
      autor,
      meta ? JSON.stringify(meta) : null,
    ];
    
    const result = await pool.query(query, values);
    logger.info('Leitura processada inserida', {
      proc_id: result.rows[0].proc_id,
      elemento_id,
      valor,
      variacao,
    });
    
    return result.rows[0];
  } catch (error) {
    logger.error('Erro ao inserir leitura processada:', error);
    throw error;
  }
}

/**
 * Atualiza data_fim da última leitura processada (compressão)
 */
export async function extendProcessedReading(procId, dataFim) {
  try {
    const query = `
      UPDATE aguada.leituras_processadas
      SET data_fim = $1
      WHERE proc_id = $2
      RETURNING *
    `;
    
    const result = await pool.query(query, [dataFim, procId]);
    logger.debug('Leitura processada estendida', { proc_id: procId });
    
    return result.rows[0];
  } catch (error) {
    logger.error('Erro ao estender leitura processada:', error);
    throw error;
  }
}

/**
 * Busca janela de leituras recentes para calcular estabilidade
 */
export async function getRecentReadings(elementoId, variavel, windowSize = 11) {
  try {
    const query = `
      SELECT valor, datetime
      FROM aguada.leituras_raw
      WHERE elemento_id = $1 AND variavel = $2
      ORDER BY datetime DESC
      LIMIT $3
    `;
    
    const result = await pool.query(query, [elementoId, variavel, windowSize]);
    return result.rows.map(r => r.valor);
  } catch (error) {
    logger.error('Erro ao buscar leituras recentes:', error);
    throw error;
  }
}

export default {
  insertRawReading,
  getLastProcessedReading,
  insertProcessedReading,
  extendProcessedReading,
  getRecentReadings,
};
