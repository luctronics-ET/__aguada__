import { Box, Container, Grid, Typography, CircularProgress, Alert } from '@mui/material';
import { useReservoirData } from '../hooks/useReservoirData';
import { RESERVOIRS, type ReservoirId } from '../types/reservoir.types';
import ReservoirCard from '../components/Dashboard/ReservoirCard';
import type { TelemetryReading } from '../types/telemetry.types';

export default function DashboardPage() {
  const { data, isLoading, isError, error } = useReservoirData();

  if (isLoading) {
    return (
      <Box sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', minHeight: '80vh' }}>
        <CircularProgress size={60} />
      </Box>
    );
  }

  if (isError) {
    return (
      <Container maxWidth="lg" sx={{ mt: 4 }}>
        <Alert severity="error">
          Erro ao carregar dados: {error instanceof Error ? error.message : 'Erro desconhecido'}
        </Alert>
      </Container>
    );
  }

  // Agrupar leituras por reservatório
  const reservoirReadings: Record<string, Record<string, TelemetryReading>> = {};
  
  if (data) {
    Object.values(data).forEach((reading) => {
      const { elemento_id, variavel } = reading;
      if (!reservoirReadings[elemento_id]) {
        reservoirReadings[elemento_id] = {};
      }
      reservoirReadings[elemento_id][variavel] = reading;
    });
  }

  return (
    <Container maxWidth="xl" sx={{ mt: 4, mb: 4 }}>
      <Box sx={{ mb: 4 }}>
        <Typography variant="h4" component="h1" gutterBottom>
          💧 Dashboard AGUADA
        </Typography>
        <Typography variant="body1" color="text.secondary">
          Sistema de Monitoramento Hidráulico em Tempo Real
        </Typography>
      </Box>

      <Grid container spacing={3}>
        {Object.keys(RESERVOIRS).map((reservoirId) => {
          const reservoir = RESERVOIRS[reservoirId as ReservoirId];
          const readings = reservoirReadings[reservoirId] || {};

          return (
            <Grid item xs={12} sm={6} md={4} lg={2.4} key={reservoirId}>
              <ReservoirCard
                reservoir={reservoir}
                readings={{
                  distance: readings.distance_cm,
                  valve_in: readings.valve_in,
                  valve_out: readings.valve_out,
                  sound_in: readings.sound_in,
                }}
              />
            </Grid>
          );
        })}
      </Grid>
    </Container>
  );
}
