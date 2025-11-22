import { Box, Typography, Grid, Card, CardContent, Paper, CircularProgress, Alert } from '@mui/material';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Legend } from 'recharts';
import { TrendingDown } from '@mui/icons-material';
import { useConsumptionStats } from '../hooks/useApi';

export default function ConsumoPage() {
  const { data: consumptionData, isLoading, isError } = useConsumptionStats({ periodo: 'week' });

  if (isLoading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="60vh">
        <CircularProgress size={60} />
      </Box>
    );
  }

  if (isError) {
    return <Alert severity="error">Erro ao carregar dados de consumo</Alert>;
  }

  const data = consumptionData?.data?.chartData || [
    { name: 'Dom', RCON: 400, RCAV: 240, RB03: 150, IE01: 280, IE02: 320 },
    { name: 'Seg', RCON: 300, RCAV: 139, RB03: 221, IE01: 290, IE02: 310 },
    { name: 'Ter', RCON: 200, RCAV: 380, RB03: 229, IE01: 300, IE02: 290 },
    { name: 'Qua', RCON: 278, RCAV: 390, RB03: 200, IE01: 310, IE02: 280 },
    { name: 'Qui', RCON: 189, RCAV: 480, RB03: 218, IE01: 290, IE02: 300 },
    { name: 'Sex', RCON: 239, RCAV: 380, RB03: 250, IE01: 310, IE02: 290 },
    { name: 'Sáb', RCON: 349, RCAV: 430, RB03: 210, IE01: 300, IE02: 310 },
  ];

  return (
    <Box>
      <Typography variant="h4" gutterBottom fontWeight="bold">
        💧 Consumo de Água
      </Typography>

      <Grid container spacing={2} sx={{ mb: 3 }}>
        {[
          { label: 'Hoje', value: '1.234 L', change: '-5%' },
          { label: 'Semana', value: '8.567 L', change: '+2%' },
          { label: 'Mês', value: '35.421 L', change: '-3%' },
        ].map((stat, i) => (
          <Grid item xs={12} md={4} key={i}>
            <Card>
              <CardContent>
                <Typography color="text.secondary">{stat.label}</Typography>
                <Typography variant="h4">{stat.value}</Typography>
                <Box display="flex" alignItems="center" gap={1}>
                  <TrendingDown fontSize="small" color={stat.change.startsWith('-') ? 'success' : 'error'} />
                  <Typography variant="body2" color={stat.change.startsWith('-') ? 'success.main' : 'error.main'}>
                    {stat.change}
                  </Typography>
                </Box>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>

      <Paper sx={{ p: 2 }}>
        <Typography variant="h6" gutterBottom>Consumo Semanal</Typography>
        <ResponsiveContainer width="100%" height={400}>
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="name" />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="RCON" stroke="#8884d8" />
            <Line type="monotone" dataKey="RCAV" stroke="#82ca9d" />
            <Line type="monotone" dataKey="RB03" stroke="#ffc658" />
            <Line type="monotone" dataKey="IE01" stroke="#ff7300" />
            <Line type="monotone" dataKey="IE02" stroke="#0088fe" />
          </LineChart>
        </ResponsiveContainer>
      </Paper>
    </Box>
  );
}
