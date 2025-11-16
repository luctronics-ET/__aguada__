import pool from '../config/database.js';
import logger from '../config/logger.js';

/**
 * Detecta evento de ABASTECIMENTO
 * Critérios: ΔV > +50L, bomba ON, válvula ABERTA, duração > 300s
 */
export async function detectSupplyEvent(elementoId, volumeAnteriorM3, volumeAtualM3, datetime) {
  try {
    const deltaVolumeL = (volumeAtualM3 - volumeAnteriorM3) * 1000;
    
    if (deltaVolumeL < 50) {
      return null; // Sem evento
    }
    
    // TODO: Verificar estado de bombas e válvulas
    // const bombaStatus = await getBombaStatus(...);
    // const valvulaStatus = await getValvulaStatus(...);
    
    const evento = {
      tipo: 'ABASTECIMENTO',
      elemento_id: elementoId,
      detalhe: {
        volume_abastecido_l: parseFloat(deltaVolumeL.toFixed(2)),
        volume_inicial_m3: volumeAnteriorM3,
        volume_final_m3: volumeAtualM3,
      },
      causa_provavel: 'Bomba de recalque ativa',
      nivel_confianca: 0.9,
      detectado_por: 'volume_engine',
      datetime_inicio: datetime,
      datetime_fim: datetime,
    };
    
    const result = await insertEvent(evento);
    logger.info('Evento ABASTECIMENTO detectado', { evento_id: result.evento_id, deltaVolumeL });
    
    return result;
  } catch (error) {
    logger.error('Erro ao detectar evento de abastecimento:', error);
    throw error;
  }
}

/**
 * Detecta evento de VAZAMENTO
 * Critérios: Queda contínua > 1h, taxa > -15L/h, sem bombeamento
 */
export async function detectLeakEvent(elementoId) {
  try {
    // Buscar leituras da última hora
    const query = `
      SELECT valor, volume_m3, data_fim
      FROM aguada.leituras_processadas
      WHERE elemento_id = $1 
        AND variavel = 'nivel_cm'
        AND data_fim > NOW() - INTERVAL '1 hour'
      ORDER BY data_fim ASC
    `;
    
    const result = await pool.query(query, [elementoId]);
    
    if (result.rows.length < 2) {
      return null; // Dados insuficientes
    }
    
    const readings = result.rows;
    const primeiro = readings[0];
    const ultimo = readings[readings.length - 1];
    
    const deltaVolumeL = (ultimo.volume_m3 - primeiro.volume_m3) * 1000;
    const duracaoH = (new Date(ultimo.data_fim) - new Date(primeiro.data_fim)) / 3600000;
    const taxaLH = deltaVolumeL / duracaoH;
    
    if (taxaLH < -15) {
      const evento = {
        tipo: 'VAZAMENTO',
        elemento_id: elementoId,
        detalhe: {
          taxa_l_h: parseFloat(taxaLH.toFixed(2)),
          volume_perdido_l: parseFloat(Math.abs(deltaVolumeL).toFixed(2)),
          duracao_h: parseFloat(duracaoH.toFixed(2)),
        },
        causa_provavel: 'Possível vazamento ou consumo anormal',
        nivel_confianca: 0.75,
        detectado_por: 'leak_detector',
        datetime_inicio: primeiro.data_fim,
        datetime_fim: ultimo.data_fim,
      };
      
      const inserted = await insertEvent(evento);
      logger.warn('⚠️ Evento VAZAMENTO detectado', { 
        evento_id: inserted.evento_id,
        taxaLH,
        elementoId,
      });
      
      return inserted;
    }
    
    return null;
  } catch (error) {
    logger.error('Erro ao detectar vazamento:', error);
    throw error;
  }
}

/**
 * Detecta NÍVEL CRÍTICO no reservatório de incêndio (CAV)
 * Critério: < 70% por mais de 10 minutos
 */
export async function detectCriticalLevel(elementoId, percentual, datetime) {
  try {
    // Verificar se é reservatório de incêndio
    const checkQuery = `
      SELECT parametros->>'tipo' as tipo
      FROM aguada.elementos
      WHERE elemento_id = $1
    `;
    
    const checkResult = await pool.query(checkQuery, [elementoId]);
    
    if (checkResult.rows[0]?.tipo !== 'incendio') {
      return null; // Não é reservatório de incêndio
    }
    
    if (percentual >= 70) {
      return null; // Nível OK
    }
    
    // Verificar se nível está baixo há mais de 10 minutos
    const historyQuery = `
      SELECT percentual, data_fim
      FROM aguada.leituras_processadas
      WHERE elemento_id = $1
        AND variavel = 'nivel_cm'
        AND data_fim > NOW() - INTERVAL '10 minutes'
      ORDER BY data_fim DESC
    `;
    
    const historyResult = await pool.query(historyQuery, [elementoId]);
    
    const todasAbaixo70 = historyResult.rows.every(r => r.percentual < 70);
    
    if (todasAbaixo70) {
      const evento = {
        tipo: 'NIVEL_CRITICO_CAV',
        elemento_id: elementoId,
        detalhe: {
          percentual_atual: percentual,
          threshold: 70,
          duracao_minutos: 10,
        },
        causa_provavel: 'Consumo elevado ou falha no abastecimento',
        nivel_confianca: 0.95,
        detectado_por: 'critical_level_monitor',
        datetime_inicio: datetime,
        datetime_fim: datetime,
      };
      
      const inserted = await insertEvent(evento);
      logger.error('🔥 CRÍTICO: Nível baixo no reservatório de incêndio', {
        evento_id: inserted.evento_id,
        percentual,
        elementoId,
      });
      
      return inserted;
    }
    
    return null;
  } catch (error) {
    logger.error('Erro ao detectar nível crítico:', error);
    throw error;
  }
}

/**
 * Insere evento no banco
 */
async function insertEvent(evento) {
  try {
    const query = `
      INSERT INTO aguada.eventos (
        tipo, elemento_id, detalhe, causa_provavel,
        nivel_confianca, detectado_por, datetime_inicio, datetime_fim
      ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
      RETURNING *
    `;
    
    const values = [
      evento.tipo,
      evento.elemento_id,
      JSON.stringify(evento.detalhe),
      evento.causa_provavel,
      evento.nivel_confianca,
      evento.detectado_por,
      evento.datetime_inicio,
      evento.datetime_fim,
    ];
    
    const result = await pool.query(query, values);
    return result.rows[0];
  } catch (error) {
    logger.error('Erro ao inserir evento:', error);
    throw error;
  }
}

export default {
  detectSupplyEvent,
  detectLeakEvent,
  detectCriticalLevel,
};
