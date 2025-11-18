import type { Reservoir } from '../types/reservoir.types';

/**
 * Calcula o volume atual em litros baseado no nível
 * @param nivel_cm - Nível em centímetros
 * @param reservoir - Dados do reservatório
 * @returns Volume em litros
 */
export function calculateVolume(nivel_cm: number, reservoir: Reservoir): number {
  if (reservoir.type === 'cylindrical') {
    // Volume cilíndrico: V = π * r² * h
    const radius_cm = (reservoir.diameter_cm || 0) / 2;
    const volume_cm3 = Math.PI * radius_cm * radius_cm * nivel_cm;
    return volume_cm3 / 1000; // Converte cm³ para litros
  } else {
    // Volume retangular: V = comprimento * largura * altura
    const volume_cm3 = (reservoir.length_cm || 0) * (reservoir.width_cm || 0) * nivel_cm;
    return volume_cm3 / 1000; // Converte cm³ para litros
  }
}

/**
 * Calcula a porcentagem de ocupação do reservatório
 * @param nivel_cm - Nível em centímetros
 * @param altura_total_cm - Altura total do reservatório
 * @returns Porcentagem (0-100)
 */
export function calculatePercentage(nivel_cm: number, altura_total_cm: number): number {
  if (altura_total_cm === 0) return 0;
  const percent = (nivel_cm / altura_total_cm) * 100;
  return Math.max(0, Math.min(100, percent)); // Limita entre 0 e 100
}

/**
 * Determina a cor do indicador baseado no nível
 * @param percentage - Porcentagem de ocupação
 * @returns Cor (success, warning, error, info)
 */
export function getLevelColor(percentage: number): 'success' | 'warning' | 'error' | 'info' {
  if (percentage < 10) return 'error';   // Vermelho - crítico
  if (percentage < 30) return 'warning'; // Amarelo - baixo
  return 'success';                      // Verde - normal
}

/**
 * Determina se o reservatório está em abastecimento
 * @param sound_detected - Som de água entrando detectado
 * @param trend - Tendência do nível ('rising', 'falling', 'stable')
 * @returns true se está abastecendo
 */
export function isRefilling(sound_detected: boolean, trend?: 'rising' | 'falling' | 'stable'): boolean {
  return sound_detected || trend === 'rising';
}
