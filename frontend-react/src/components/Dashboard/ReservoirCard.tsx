import { Card, CardContent, Typography, Box, Chip, LinearProgress } from '@mui/material';
import { Water, PowerSettingsNew, VolumeUp } from '@mui/icons-material';
import type { TelemetryReading } from '../../types/telemetry.types';
import type { Reservoir } from '../../types/reservoir.types';
import { calculateVolume, calculatePercentage, getLevelColor } from '../../utils/calculations';
import { formatLiters, formatPercent, formatRelativeTime, formatRSSI, formatBattery } from '../../utils/formatters';

interface ReservoirCardProps {
  reservoir: Reservoir;
  readings: {
    distance?: TelemetryReading;
    valve_in?: TelemetryReading;
    valve_out?: TelemetryReading;
    sound_in?: TelemetryReading;
  };
}

export default function ReservoirCard({ reservoir, readings }: ReservoirCardProps) {
  // Calcular nível e volume
  const distance_cm = readings.distance?.valor || 0;
  const nivel_cm = reservoir.height_cm - distance_cm;
  const percentage = calculatePercentage(nivel_cm, reservoir.height_cm);
  const volume_liters = calculateVolume(nivel_cm, reservoir);
  const volume_m3 = volume_liters / 1000;
  
  // Estados das válvulas e som
  const valve_in_open = readings.valve_in?.valor === 1;
  const valve_out_open = readings.valve_out?.valor === 1;
  const sound_detected = readings.sound_in?.valor === 1;
  
  // Metadados
  const rssi = readings.distance?.meta?.rssi_dbm || 0;
  const battery = readings.distance?.meta?.battery_mv || 0;
  const lastUpdate = readings.distance?.datetime || '';
  
  // Cor do nível
  const levelColor = getLevelColor(percentage);
  
  return (
    <Card 
      sx={{ 
        height: '100%',
        borderTop: 4,
        borderColor: `${levelColor}.main`,
      }}
    >
      <CardContent>
        {/* Cabeçalho */}
        <Box sx={{ mb: 2 }}>
          <Typography variant="h6" component="div" gutterBottom>
            <Water sx={{ mr: 1, verticalAlign: 'middle' }} />
            {reservoir.name}
          </Typography>
          <Typography variant="caption" color="text.secondary">
            {reservoir.id}
          </Typography>
        </Box>

        {/* Gauge de nível */}
        <Box sx={{ textAlign: 'center', mb: 2 }}>
          <Typography variant="h3" color={`${levelColor}.main`} sx={{ fontWeight: 'bold' }}>
            {formatPercent(percentage)}
          </Typography>
          <LinearProgress 
            variant="determinate" 
            value={percentage} 
            color={levelColor}
            sx={{ height: 10, borderRadius: 5, mt: 1 }}
          />
        </Box>

        {/* Volume */}
        <Box sx={{ mb: 2 }}>
          <Typography variant="body2" color="text.secondary">
            Volume Atual
          </Typography>
          <Typography variant="h6">
            {volume_m3.toFixed(1)} m³
          </Typography>
          <Typography variant="caption" color="text.secondary">
            {formatLiters(volume_liters)}
          </Typography>
        </Box>

        {/* Estados */}
        <Box sx={{ mb: 2, display: 'flex', gap: 1, flexWrap: 'wrap' }}>
          <Chip 
            label={`Entrada ${valve_in_open ? 'Aberta' : 'Fechada'}`}
            color={valve_in_open ? 'success' : 'default'}
            size="small"
            icon={<PowerSettingsNew />}
          />
          <Chip 
            label={`Saída ${valve_out_open ? 'Aberta' : 'Fechada'}`}
            color={valve_out_open ? 'success' : 'default'}
            size="small"
            icon={<PowerSettingsNew />}
          />
          <Chip 
            label={sound_detected ? 'Som Detectado' : 'Sem Som'}
            color={sound_detected ? 'info' : 'default'}
            size="small"
            icon={<VolumeUp />}
          />
        </Box>

        {/* Metadados */}
        <Box sx={{ mt: 2, pt: 2, borderTop: 1, borderColor: 'divider' }}>
          <Typography variant="caption" display="block" color="text.secondary">
            {formatRSSI(rssi)} • {formatBattery(battery)}
          </Typography>
          <Typography variant="caption" display="block" color="text.secondary">
            Atualizado {formatRelativeTime(lastUpdate)}
          </Typography>
        </Box>
      </CardContent>
    </Card>
  );
}
