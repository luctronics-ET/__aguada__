import { useState } from 'react';
import {
  Box,
  Paper,
  Typography,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Chip,
  TextField,
  MenuItem,
  Grid,
  Card,
  CardContent,
  CircularProgress,
  Alert,
} from '@mui/material';
import { WaterDrop, Sensors, Speed, Check } from '@mui/icons-material';
import { useRawReadings } from '../hooks/useApi';

interface Reading {
  leitura_id: number;
  datetime: string;
  elemento_id: string;
  variavel: string;
  valor: number;
  unidade: string;
  sensor_id: string;
  meta: any;
}

export default function DadosPage() {
  const [filter, setFilter] = useState('all');
  const { data: readingsData, isLoading, isError, error } = useRawReadings({ limit: 100 });

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

  const readings: Reading[] = readingsData?.data || [];
  
  // Determinar status baseado no valor
  const getStatus = (variavel: string, valor: number): 'normal' | 'warning' | 'critical' => {
    if (variavel === 'nivel_cm' || variavel === 'distance_cm') {
      const nivel = valor / 100; // Valor vem multiplicado por 100
      if (nivel < 50) return 'critical';
      if (nivel < 100) return 'warning';
    }
    return 'normal';
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'normal': return 'success';
      case 'warning': return 'warning';
      case 'critical': return 'error';
      default: return 'default';
    }
  };

  // Calcular estatísticas
  const readingsWithStatus = readings.map(r => ({
    ...r,
    status: getStatus(r.variavel, r.valor)
  }));

  const stats = {
    total: readings.length,
    normal: readingsWithStatus.filter(r => r.status === 'normal').length,
    warning: readingsWithStatus.filter(r => r.status === 'warning').length,
    critical: readingsWithStatus.filter(r => r.status === 'critical').length,
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom fontWeight="bold">
        📊 Dados em Tempo Real
      </Typography>
      <Typography variant="body1" color="text.secondary" paragraph>
        Leituras dos sensores de todos os reservatórios
      </Typography>

      {/* Estatísticas */}
      <Grid container spacing={2} sx={{ mb: 3 }}>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent>
              <Box display="flex" alignItems="center" justifyContent="space-between">
                <Box>
                  <Typography color="text.secondary" variant="body2">Total</Typography>
                  <Typography variant="h4">{stats.total}</Typography>
                </Box>
                <Sensors sx={{ fontSize: 40, color: 'primary.main' }} />
              </Box>
            </CardContent>
          </Card>
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent>
              <Box display="flex" alignItems="center" justifyContent="space-between">
                <Box>
                  <Typography color="text.secondary" variant="body2">Normal</Typography>
                  <Typography variant="h4">{stats.normal}</Typography>
                </Box>
                <Check sx={{ fontSize: 40, color: 'success.main' }} />
              </Box>
            </CardContent>
          </Card>
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent>
              <Box display="flex" alignItems="center" justifyContent="space-between">
                <Box>
                  <Typography color="text.secondary" variant="body2">Aviso</Typography>
                  <Typography variant="h4">{stats.warning}</Typography>
                </Box>
                <Speed sx={{ fontSize: 40, color: 'warning.main' }} />
              </Box>
            </CardContent>
          </Card>
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent>
              <Box display="flex" alignItems="center" justifyContent="space-between">
                <Box>
                  <Typography color="text.secondary" variant="body2">Crítico</Typography>
                  <Typography variant="h4">{stats.critical}</Typography>
                </Box>
                <WaterDrop sx={{ fontSize: 40, color: 'error.main' }} />
              </Box>
            </CardContent>
          </Card>
        </Grid>
      </Grid>

      {/* Filtros */}
      <Box sx={{ mb: 2 }}>
        <TextField
          select
          label="Filtrar por Status"
          value={filter}
          onChange={(e) => setFilter(e.target.value)}
          sx={{ minWidth: 200 }}
        >
          <MenuItem value="all">Todos</MenuItem>
          <MenuItem value="normal">Normal</MenuItem>
          <MenuItem value="warning">Aviso</MenuItem>
          <MenuItem value="critical">Crítico</MenuItem>
        </TextField>
      </Box>

      {/* Tabela de Dados */}
      <TableContainer component={Paper}>
        <Table>
          <TableHead>
            <TableRow>
              <TableCell><strong>Timestamp</strong></TableCell>
              <TableCell><strong>Reservatório</strong></TableCell>
              <TableCell><strong>Variável</strong></TableCell>
              <TableCell align="right"><strong>Valor</strong></TableCell>
              <TableCell><strong>Unidade</strong></TableCell>
              <TableCell><strong>Status</strong></TableCell>
            </TableRow>
          </TableHead>
          <TableBody>
            {readingsWithStatus
              .filter(r => filter === 'all' || r.status === filter)
              .map((row) => (
                <TableRow key={row.leitura_id} hover>
                  <TableCell>{new Date(row.datetime).toLocaleString('pt-BR')}</TableCell>
                  <TableCell><Chip label={row.elemento_id} size="small" /></TableCell>
                  <TableCell>{row.variavel}</TableCell>
                  <TableCell align="right"><strong>{(row.valor / 100).toFixed(2)}</strong></TableCell>
                  <TableCell>{row.unidade || 'cm'}</TableCell>
                  <TableCell>
                    <Chip
                      label={row.status.toUpperCase()}
                      color={getStatusColor(row.status) as any}
                      size="small"
                    />
                  </TableCell>
                </TableRow>
              ))}
          </TableBody>
        </Table>
      </TableContainer>
    </Box>
  );
}
