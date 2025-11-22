import { Box, Typography, Paper, Grid, Card, CardContent } from '@mui/material';

export default function MapaPage() {
  const reservoirs = [
    { id: 'RCON', name: 'Reservatório Condomínio', x: 100, y: 100, level: 75 },
    { id: 'RCAV', name: 'Reservatório Cavalinho', x: 300, y: 150, level: 45 },
    { id: 'RB03', name: 'Reservatório B03', x: 500, y: 100, level: 20 },
    { id: 'IE01', name: 'Ilha Engenho 01', x: 200, y: 300, level: 85 },
    { id: 'IE02', name: 'Ilha Engenho 02', x: 400, y: 300, level: 70 },
  ];

  const getColor = (level: number) => {
    if (level < 20) return '#f44336';
    if (level < 50) return '#ff9800';
    return '#4caf50';
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom fontWeight="bold">
        🗺️ Mapa da Rede
      </Typography>

      <Paper sx={{ p: 2, mb: 3 }}>
        <svg width="100%" height="400" viewBox="0 0 600 400">
          {/* Conexões */}
          <line x1="100" y1="100" x2="300" y2="150" stroke="#ccc" strokeWidth="2" />
          <line x1="300" y1="150" x2="500" y2="100" stroke="#ccc" strokeWidth="2" />
          <line x1="100" y1="100" x2="200" y2="300" stroke="#ccc" strokeWidth="2" />
          <line x1="200" y1="300" x2="400" y2="300" stroke="#ccc" strokeWidth="2" />

          {/* Reservatórios */}
          {reservoirs.map((r) => (
            <g key={r.id}>
              <circle cx={r.x} cy={r.y} r="30" fill={getColor(r.level)} opacity="0.8" />
              <text x={r.x} y={r.y} textAnchor="middle" fill="white" fontWeight="bold">{r.id}</text>
              <text x={r.x} y={r.y + 15} textAnchor="middle" fill="white" fontSize="12">{r.level}%</text>
            </g>
          ))}
        </svg>
      </Paper>

      <Grid container spacing={2}>
        {reservoirs.map((r) => (
          <Grid item xs={12} sm={6} md={4} key={r.id}>
            <Card>
              <CardContent>
                <Typography variant="h6">{r.id}</Typography>
                <Typography color="text.secondary" variant="body2">{r.name}</Typography>
                <Typography variant="h4" sx={{ color: getColor(r.level) }}>{r.level}%</Typography>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>
    </Box>
  );
}
