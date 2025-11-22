import { Box, Typography, Grid, Card, CardContent, Chip, List, ListItem, ListItemText, Paper, Button, CircularProgress } from '@mui/material';
import { CheckCircle, Error, Refresh, Save, Delete } from '@mui/icons-material';
import { useSystemHealth, useSystemMetrics, useSystemLogs } from '../hooks/useApi';

export default function SystemPage() {
  const { data: healthData, isLoading: healthLoading } = useSystemHealth();
  const { data: metricsData } = useSystemMetrics();
  const { data: logsData } = useSystemLogs({ limit: 20 });

  if (healthLoading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="60vh">
        <CircularProgress size={60} />
      </Box>
    );
  }

  const services = healthData?.data?.services || [];
  const stats = metricsData?.data?.metrics || {};
  const logs = logsData?.data || [];

  return (
    <Box>
      <Typography variant="h4" gutterBottom fontWeight="bold">
        ⚙️ Status do Sistema
      </Typography>

      {/* Status dos Serviços */}
      <Typography variant="h6" gutterBottom sx={{ mt: 3 }}>Serviços</Typography>
      <Grid container spacing={2} sx={{ mb: 3 }}>
        {services.map((service: any) => (
          <Grid item xs={12} sm={6} md={3} key={service.name}>
            <Card>
              <CardContent>
                <Box display="flex" alignItems="center" justifyContent="space-between" mb={1}>
                  <Typography variant="h6">{service.name}</Typography>
                  {service.status === 'online' ? (
                    <CheckCircle color="success" />
                  ) : (
                    <Error color="error" />
                  )}
                </Box>
                <Chip
                  label={service.status.toUpperCase()}
                  color={service.status === 'online' ? 'success' : 'error'}
                  size="small"
                />
                <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                  Uptime: {service.uptime}
                </Typography>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>

      {/* Estatísticas */}
      <Typography variant="h6" gutterBottom>Estatísticas</Typography>
      <Grid container spacing={2} sx={{ mb: 3 }}>
        {Array.isArray(stats) && stats.map((stat: any, i: number) => (
          <Grid item xs={6} md={3} key={i}>
            <Card>
              <CardContent>
                <Typography color="text.secondary">{stat.label}</Typography>
                <Typography variant="h4">{stat.value}</Typography>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>

      {/* Ações */}
      <Typography variant="h6" gutterBottom>Ações</Typography>
      <Box display="flex" gap={2} mb={3}>
        <Button variant="contained" startIcon={<Refresh />}>Restart</Button>
        <Button variant="outlined" startIcon={<Save />}>Backup</Button>
        <Button variant="outlined" color="error" startIcon={<Delete />}>Clear Cache</Button>
      </Box>

      {/* Logs */}
      <Typography variant="h6" gutterBottom>Logs Recentes</Typography>
      <Paper>
        <List>
          {logs.map((log: any, i: number) => (
            <ListItem key={i} divider>
              <ListItemText
                primary={
                  <Box display="flex" alignItems="center" gap={1}>
                    <Typography variant="body2" color="text.secondary">{log.time}</Typography>
                    <Chip label={log.level} size="small" color={log.level === 'WARN' ? 'warning' : 'default'} />
                    <Typography>{log.message}</Typography>
                  </Box>
                }
              />
            </ListItem>
          ))}
        </List>
      </Paper>
    </Box>
  );
}
