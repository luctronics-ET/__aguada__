import redisClient from '../config/redis.js';
import logger from '../config/logger.js';

const CACHE_TTL = 5; // 5 segundos
const CACHE_PREFIX = 'aguada:';

/**
 * Cache service para otimizar queries frequentes
 */
class CacheService {
  /**
   * Obtém valor do cache
   */
  async get(key) {
    try {
      if (!redisClient.isOpen) {
        await redisClient.connect();
      }
      
      const cached = await redisClient.get(`${CACHE_PREFIX}${key}`);
      if (cached) {
        return JSON.parse(cached);
      }
      return null;
    } catch (error) {
      logger.error('Erro ao obter do cache:', error);
      return null;
    }
  }

  /**
   * Define valor no cache com TTL
   */
  async set(key, value, ttl = CACHE_TTL) {
    try {
      if (!redisClient.isOpen) {
        await redisClient.connect();
      }
      
      await redisClient.setEx(
        `${CACHE_PREFIX}${key}`,
        ttl,
        JSON.stringify(value)
      );
    } catch (error) {
      logger.error('Erro ao definir cache:', error);
    }
  }

  /**
   * Remove chave do cache
   */
  async del(key) {
    try {
      if (!redisClient.isOpen) {
        await redisClient.connect();
      }
      
      await redisClient.del(`${CACHE_PREFIX}${key}`);
    } catch (error) {
      logger.error('Erro ao remover do cache:', error);
    }
  }

  /**
   * Remove múltiplas chaves (pattern)
   */
  async delPattern(pattern) {
    try {
      if (!redisClient.isOpen) {
        await redisClient.connect();
      }
      
      const keys = await redisClient.keys(`${CACHE_PREFIX}${pattern}`);
      if (keys.length > 0) {
        await redisClient.del(keys);
      }
    } catch (error) {
      logger.error('Erro ao remover padrão do cache:', error);
    }
  }

  /**
   * Invalida cache de leituras
   */
  async invalidateReadings() {
    await this.delPattern('readings:*');
    logger.debug('Cache de leituras invalidado');
  }

  /**
   * Invalida cache de sensor específico
   */
  async invalidateSensor(sensorId) {
    await this.delPattern(`readings:*:${sensorId}*`);
    await this.del('readings:latest');
    logger.debug(`Cache do sensor ${sensorId} invalidado`);
  }
}

export default new CacheService();

