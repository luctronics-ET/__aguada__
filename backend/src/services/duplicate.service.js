import redisClient from '../config/redis.js';
import logger from '../config/logger.js';

const DUPLICATE_WINDOW_MS = 1000; // 1 segundo
const DUPLICATE_PREFIX = 'dup:';

/**
 * Serviço para detectar e prevenir duplicatas de leituras
 */
class DuplicateService {
  /**
   * Gera hash único para uma leitura
   */
  generateHash(sensorId, datetime, valor) {
    // Arredondar datetime para segundo (remover milissegundos)
    const roundedTime = new Date(datetime);
    roundedTime.setMilliseconds(0);
    
    // Hash: sensor_id + timestamp (segundo) + valor arredondado
    const timestamp = Math.floor(roundedTime.getTime() / 1000);
    const roundedValue = Math.round(parseFloat(valor) * 100) / 100; // 2 casas decimais
    
    return `${sensorId}:${timestamp}:${roundedValue}`;
  }

  /**
   * Verifica se uma leitura é duplicada
   * Retorna true se for duplicada, false caso contrário
   */
  async isDuplicate(sensorId, datetime, valor) {
    try {
      if (!redisClient.isOpen) {
        await redisClient.connect();
      }

      const hash = this.generateHash(sensorId, datetime, valor);
      const key = `${DUPLICATE_PREFIX}${hash}`;

      // Verificar se já existe
      const exists = await redisClient.exists(key);
      
      if (exists) {
        logger.debug('Leitura duplicada detectada', {
          sensor_id: sensorId,
          hash,
          datetime,
          valor,
        });
        return true;
      }

      // Marcar como processada (TTL de 2 segundos para garantir)
      await redisClient.setEx(key, 2, '1');

      return false;
    } catch (error) {
      logger.error('Erro ao verificar duplicata:', error);
      // Em caso de erro, permitir a leitura (fail open)
      return false;
    }
  }

  /**
   * Limpa hashes antigos (manutenção periódica)
   */
  async cleanup() {
    try {
      if (!redisClient.isOpen) {
        await redisClient.connect();
      }

      // Buscar todas as chaves de duplicata
      const keys = await redisClient.keys(`${DUPLICATE_PREFIX}*`);
      
      // Redis já remove automaticamente com TTL, mas podemos forçar limpeza
      // de chaves órfãs (não deve ser necessário, mas é uma segurança)
      if (keys.length > 10000) {
        logger.warn('Muitas chaves de duplicata no Redis, considerando limpeza manual');
      }

      logger.debug('Limpeza de duplicatas concluída', { keys: keys.length });
    } catch (error) {
      logger.error('Erro na limpeza de duplicatas:', error);
    }
  }
}

export default new DuplicateService();

