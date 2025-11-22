import { Box, Grid, Typography, Card, CardContent, LinearProgress, Chip, CircularProgress, Alert } from '@mui/material';
import { WaterDrop, Speed, TrendingUp, TrendingDown } from '@mui/icons-material';
import { useLatestReadings } from '../hooks/useApi';

interface ReservoirData {
  id: string;
  name: string;
  level: number;
  volume: number;
  capacity: number;
  status: 'normal' | 'warning' | 'critical';
  valveIn: boolean;
  valveOut: boolean;
}

// Configuração dos reservatórios (baseado em RULES.md)
const RESERVOIR_CONFIG: Record<string, { name: string; capacity: number; height: number }> = {
  RCON: { name: 'Reservatório Condomínio', capacity: 20000, height: 400 },
  RCAV: { name: 'Reservatório Cavalinho', capacity: 10000, height: 300 },
  RB03: { name: 'Reservatório B03', capacity: 10000, height: 300 },
  IE01: { name: 'Ilha Engenho 01', capacity: 20000, height: 400 },
  IE02: { name: 'Ilha Engenho 02', capacity: 20000, height: 400 },
};

export default function DashboardPage() {
  const { data: latestReadings, isLoading, isError, error } = useLatestReadings();

  if (isLoading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="60vh">
        <CircularProgress size={60} />
      </Box>
    );
  }

  if (isError) {
    return (
      <Alert severity="error" sx={{ mt: 2 }}>
        Erro ao carregar dados: {error instanceof Error ? error.message : 'Erro desconhecido'}
      </Alert>
    );
  }

  // Processar leituras para agrupar por reservatório
  const reservoirs: ReservoirData[] = Object.keys(RESERVOIR_CONFIG).map((reservoirId) => {
    const config = RESERVOIR_CONFIG[reservoirId];
    
    // Buscar leituras deste reservatório
    const readings = latestReadings?.data?.filter((r: any) => r.elemento_id === reservoirId) || [];
    
    const distanceReading = readings.find((r: any) => r.variavel === 'nivel_cm' || r.variavel === 'distance_cm');
    const valveInReading = readings.find((r: any) => r.variavel === 'valve_in');
    const valveOutReading = readings.find((r: any) => r.variavel === 'valve_out');

    // Calcular nível e volume
    let level = 0;
    let volume = 0;
    
    if (distanceReading) {
      const distanceCm = distanceReading.valor / 100; // valor vem multiplicado por 100
      const waterHeightCm = config.height - distanceCm;
      level = (waterHeightCm / config.height) * 100;
      volume = (waterHeightCm / config.height) * config.capacity;
    }

    // Determinar status
    let status: 'normal' | 'warning' | 'critical' = 'normal';
    if (level < 10) status = 'critical';
    else if (level < 20) status = 'warning';

    return {
      id: reservoirId,
      name: config.name,
      level: Math.max(0, Math.min(100, level)),
      volume,
      capacity: config.capacity,
      status,
      valveIn: valveInReading?.valor === 1 || false,
      valveOut: valveOutReading?.valor === 1 || false,
    };
  });

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'normal': return 'success';
      case 'warning': return 'warning';
      case 'critical': return 'error';
      default: return 'default';
    }
  };

  const getProgressColor = (level: number) => {
    if (level < 20) return 'error';
    if (level < 50) return 'warning';
    return 'success';
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom fontWeight="bold">
        💧 Dashboard AGUADA
      </Typography>
      <Typography variant="body1" color="text.secondary" paragraph>
        Sistema de Monitoramento Hidráulico em Tempo Real
      </Typography>

      <Grid container spacing={3}>
        {reservoirs.map((reservoir) => (
          <Grid item xs={12} sm={6} md={4} key={reservoir.id}>
            <Card sx={{ height: '100%' }}>
              <CardContent>
                {/* Header */}
                <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                  <Box>
                    <Typography variant="h6" fontWeight="bold">
                      {reservoir.id}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      {reservoir.name}
                    </Typography>
                  </Box>
                  <WaterDrop sx={{ fontSize: 40, color: `${getStatusColor(reservoir.status)}.main` }} />
                </Box>

                {/* Status */}
                <Box mb={2}>
                  <Chip
                    label={reservoir.status.toUpperCase()}
                    color={getStatusColor(reservoir.status) as any}
                    size="small"
                  />
                </Box>

                {/* Nível */}
                <Box mb={2}>
                  <Box display="flex" justifyContent="space-between" alignItems="center" mb={1}>
                    <Typography variant="body2" color="text.secondary">
                      Nível
                    </Typography>
                    <Typography variant="h5" fontWeight="bold">
                      {reservoir.level}%
                    </Typography>
                  </Box>
                  <LinearProgress
                    variant="determinate"
                    value={reservoir.level}
                    color={getProgressColor(reservoir.level)}
                    sx={{ height: 10, borderRadius: 5 }}
                  />
                </Box>

                {/* Volume */}
                <Box mb={2}>
                  <Typography variant="body2" color="text.secondary">
                    Volume
                  </Typography>
                  <Typography variant="h6">
                    {(reservoir.volume / 1000).toFixed(1)} m³ / {(reservoir.capacity / 1000).toFixed(0)} m³
                  </Typography>
                </Box>

                {/* Válvulas */}
                <Box display="flex" gap={1}>
                  <Chip
                    label="Entrada"
                    size="small"
                    color={reservoir.valveIn ? 'success' : 'default'}
                    icon={reservoir.valveIn ? <TrendingUp /> : <Speed />}
                  />
                  <Chip
                    label="Saída"
                    size="small"
                    color={reservoir.valveOut ? 'primary' : 'default'}
                    icon={reservoir.valveOut ? <TrendingDown /> : <Speed />}
                  />
                </Box>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>
    </Box>
  );
}
