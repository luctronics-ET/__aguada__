// Tipos para os reservatórios e elementos hidráulicos
export interface Reservoir {
  id: string;
  name: string;
  type: 'cylindrical' | 'rectangular';
  capacity_m3: number;
  height_cm: number;
  diameter_cm?: number;
  length_cm?: number;
  width_cm?: number;
}

// Mapeamento dos reservatórios do sistema AGUADA
export const RESERVOIRS: Record<string, Reservoir> = {
  RCON: {
    id: 'RCON',
    name: 'Castelo de Consumo',
    type: 'cylindrical',
    capacity_m3: 80,
    height_cm: 400,
    diameter_cm: 510,
  },
  RCAV: {
    id: 'RCAV',
    name: 'Castelo de Incêndio',
    type: 'cylindrical',
    capacity_m3: 80,
    height_cm: 400,
    diameter_cm: 510,
  },
  RB03: {
    id: 'RB03',
    name: 'Reservatório Casa de Bombas',
    type: 'cylindrical',
    capacity_m3: 80,
    height_cm: 400,
    diameter_cm: 510,
  },
  IE01: {
    id: 'IE01',
    name: 'Cisterna IE 01',
    type: 'rectangular',
    capacity_m3: 254,
    height_cm: 240,
    length_cm: 585,
    width_cm: 1810,
  },
  IE02: {
    id: 'IE02',
    name: 'Cisterna IE 02',
    type: 'rectangular',
    capacity_m3: 254,
    height_cm: 240,
    length_cm: 585,
    width_cm: 1810,
  },
};

export type ReservoirId = keyof typeof RESERVOIRS;
