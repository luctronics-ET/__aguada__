import logger from '../config/logger.js';

/**
 * Calcula volume baseado no nível e parâmetros do elemento
 */
export function calculateVolume(nivelCm, elementoParametros) {
  try {
    const { forma, altura_cm, diametro_cm, largura_cm, comprimento_cm, offset_cm } = elementoParametros;
    
    // Aplicar offset (distância do sensor ao topo)
    const nivelReal = nivelCm - (offset_cm || 0);
    
    if (nivelReal < 0) {
      logger.warn('Nível calculado negativo após offset', { nivelCm, offset_cm });
      return { volume_m3: 0, percentual: 0 };
    }
    
    let volumeM3 = 0;
    
    if (forma === 'cilindrica') {
      // V = π * r² * h
      const raio = (diametro_cm / 2) / 100; // converter para metros
      const altura = nivelReal / 100;
      volumeM3 = Math.PI * Math.pow(raio, 2) * altura;
      
    } else if (forma === 'retangular') {
      // V = l * w * h
      const l = (largura_cm || 0) / 100;
      const w = (comprimento_cm || 0) / 100;
      const h = nivelReal / 100;
      volumeM3 = l * w * h;
      
    } else {
      logger.warn('Forma geométrica desconhecida', { forma });
      return { volume_m3: 0, percentual: 0 };
    }
    
    // Calcular percentual
    const alturaMaxima = (altura_cm || 0) - (offset_cm || 0);
    const percentual = alturaMaxima > 0 ? (nivelReal / alturaMaxima) * 100 : 0;
    
    return {
      volume_m3: parseFloat(volumeM3.toFixed(3)),
      percentual: parseFloat(Math.min(percentual, 100).toFixed(2)),
    };
    
  } catch (error) {
    logger.error('Erro ao calcular volume:', error);
    return { volume_m3: 0, percentual: 0 };
  }
}

/**
 * Calcula mediana de um array de valores
 */
export function calculateMedian(values) {
  if (values.length === 0) return 0;
  
  const sorted = [...values].sort((a, b) => a - b);
  const mid = Math.floor(sorted.length / 2);
  
  return sorted.length % 2 === 0
    ? (sorted[mid - 1] + sorted[mid]) / 2
    : sorted[mid];
}

/**
 * Calcula desvio padrão
 */
export function calculateStdDev(values) {
  if (values.length === 0) return 0;
  
  const mean = values.reduce((a, b) => a + b, 0) / values.length;
  const variance = values.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) / values.length;
  
  return Math.sqrt(variance);
}

export default {
  calculateVolume,
  calculateMedian,
  calculateStdDev,
};
