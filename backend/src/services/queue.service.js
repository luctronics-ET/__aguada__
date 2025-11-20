import { Queue, Worker } from 'bullmq';
import redisClient from '../config/redis.js';
import readingService from './reading.service.js';
import compressionService from './compression.service.js';
import cacheService from './cache.service.js';
import logger from '../config/logger.js';

// Configuração da conexão Redis para BullMQ
const connection = {
  host: process.env.REDIS_HOST || 'localhost',
  port: parseInt(process.env.REDIS_PORT || '6379'),
};

/**
 * Fila para processamento de leituras
 */
export const readingsQueue = new Queue('readings-processing', {
  connection,
  defaultJobOptions: {
    attempts: 3,
    backoff: {
      type: 'exponential',
      delay: 2000, // 2s, 4s, 8s
    },
    removeOnComplete: {
      age: 3600, // Manter jobs completos por 1 hora
      count: 1000, // Manter últimos 1000 jobs
    },
    removeOnFail: {
      age: 86400, // Manter jobs falhos por 24 horas
    },
  },
});

/**
 * Worker para processar leituras assincronamente
 */
export const readingsWorker = new Worker(
  'readings-processing',
  async (job) => {
    const { sensor, valorReal, elementoParametros, datetime, type } = job.data;
    
    logger.info('Processando leitura na fila', {
      jobId: job.id,
      sensor_id: sensor.sensor_id,
      type,
    });

    try {
      // Processar compressão se for distance_cm
      if (type === 'distance_cm') {
        await compressionService.processCompression(
          sensor,
          valorReal,
          elementoParametros,
          datetime
        );
      }

      // Invalidar cache
      await cacheService.invalidateReadings();

      logger.info('Leitura processada com sucesso', {
        jobId: job.id,
        sensor_id: sensor.sensor_id,
      });

      return { success: true };
    } catch (error) {
      logger.error('Erro ao processar leitura na fila:', error);
      throw error; // Re-throw para retry automático
    }
  },
  {
    connection,
    concurrency: 5, // Processar até 5 leituras simultaneamente
    limiter: {
      max: 100, // Máximo 100 jobs
      duration: 1000, // por segundo
    },
  }
);

// Event handlers do worker
readingsWorker.on('completed', (job) => {
  logger.debug('Job completado', { jobId: job.id });
});

readingsWorker.on('failed', (job, err) => {
  logger.error('Job falhou', {
    jobId: job?.id,
    error: err.message,
    attempts: job?.attemptsMade,
  });
});

readingsWorker.on('error', (err) => {
  logger.error('Erro no worker:', err);
});

/**
 * Adiciona leitura à fila para processamento assíncrono
 */
export async function enqueueReading(data) {
  try {
    const job = await readingsQueue.add('process-reading', data, {
      priority: data.type === 'distance_cm' ? 1 : 2, // distance_cm tem prioridade maior
    });
    
    logger.debug('Leitura adicionada à fila', {
      jobId: job.id,
      sensor_id: data.sensor.sensor_id,
    });
    
    return job;
  } catch (error) {
    logger.error('Erro ao adicionar leitura à fila:', error);
    throw error;
  }
}

/**
 * Obtém estatísticas da fila
 */
export async function getQueueStats() {
  try {
    const [waiting, active, completed, failed] = await Promise.all([
      readingsQueue.getWaitingCount(),
      readingsQueue.getActiveCount(),
      readingsQueue.getCompletedCount(),
      readingsQueue.getFailedCount(),
    ]);

    return {
      waiting,
      active,
      completed,
      failed,
      total: waiting + active + completed + failed,
    };
  } catch (error) {
    logger.error('Erro ao obter estatísticas da fila:', error);
    return null;
  }
}

export default {
  readingsQueue,
  readingsWorker,
  enqueueReading,
  getQueueStats,
};

