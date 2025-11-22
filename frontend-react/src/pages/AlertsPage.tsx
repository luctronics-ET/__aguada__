import { Box, Typography, Paper, List, ListItem, ListItemText, Chip, Grid, Card, CardContent, CircularProgress, Alert as MuiAlert } from '@mui/material';
import { Warning, Error, Info } from '@mui/icons-material';
import { useAlerts, useAlertsSummary } from '../hooks/useApi';

export default function AlertsPage() {
  const { data: alertsData, isLoading, isError } = useAlerts({ limit: 50 });
  const { data: summaryData } = useAlertsSummary();

  if (isLoading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="60vh">
        <CircularProgress size={60} />
      </Box>
    );
  }

  if (isError) {
    return <MuiAlert severity="error">Erro ao carregar alertas</MuiAlert>;
  }

  const alerts = alertsData?.data || [];
  const summary = summaryData?.data || { critical: 0, warning: 0, info: 0 };

  return (
    <Box>
      <Typography variant="h4" gutterBottom fontWeight="bold">
        ⚠️ Alertas
      </Typography>
      
      <Grid container spacing={2} sx={{ mb: 3 }}>
        <Grid item xs={12} md={4}>
          <Card><CardContent>
            <Box display="flex" alignItems="center" gap={2}>
              <Error sx={{ fontSize: 40, color: 'error.main' }} />
              <Box>
                <Typography variant="h4">{summary.critical || 0}</Typography>
                <Typography color="text.secondary">Críticos</Typography>
              </Box>
            </Box>
          </CardContent></Card>
        </Grid>
        <Grid item xs={12} md={4}>
          <Card><CardContent>
            <Box display="flex" alignItems="center" gap={2}>
              <Warning sx={{ fontSize: 40, color: 'warning.main' }} />
              <Box>
                <Typography variant="h4">{summary.warning || 0}</Typography>
                <Typography color="text.secondary">Avisos</Typography>
              </Box>
            </Box>
          </CardContent></Card>
        </Grid>
        <Grid item xs={12} md={4}>
          <Card><CardContent>
            <Box display="flex" alignItems="center" gap={2}>
              <Info sx={{ fontSize: 40, color: 'info.main' }} />
              <Box>
                <Typography variant="h4">{summary.info || 0}</Typography>
                <Typography color="text.secondary">Informações</Typography>
              </Box>
            </Box>
          </CardContent></Card>
        </Grid>
      </Grid>

      <Paper>
        <List>
          {alerts.map((alert: any) => (
            <ListItem key={alert.anomalia_id || alert.evento_id} divider>
              <ListItemText
                primary={
                  <Box display="flex" alignItems="center" gap={1}>
                    <Chip label={alert.elemento_id} size="small" />
                    <Chip label={alert.nivel_alerta || alert.tipo} color={alert.nivel_alerta === 'CRITICO' ? 'error' : alert.nivel_alerta === 'MODERADO' ? 'warning' : 'info'} size="small" />
                    <Typography>{alert.descricao || alert.tipo}</Typography>
                  </Box>
                }
                secondary={new Date(alert.inicio || alert.datetime_inicio).toLocaleString('pt-BR')}
              />
            </ListItem>
          ))}
        </List>
      </Paper>
    </Box>
  );
}
