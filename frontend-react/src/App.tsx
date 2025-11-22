import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ThemeProvider, createTheme, CssBaseline } from '@mui/material';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import Layout from './components/Layout/Layout';
import DashboardPage from './pages/DashboardPage';
import DadosPage from './pages/DadosPage';
import AlertsPage from './pages/AlertsPage';
import ConsumoPage from './pages/ConsumoPage';
import MapaPage from './pages/MapaPage';
import SystemPage from './pages/SystemPage';

// Configurar React Query
const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      refetchOnWindowFocus: false,
      retry: 3,
    },
  },
});

// Tema customizado AGUADA
const theme = createTheme({
  palette: {
    primary: {
      main: '#1976d2', // Azul água
    },
    secondary: {
      main: '#0d47a1', // Azul escuro
    },
    success: {
      main: '#4caf50', // Verde
    },
    warning: {
      main: '#ff9800', // Laranja
    },
    error: {
      main: '#f44336', // Vermelho
    },
    info: {
      main: '#2196f3', // Azul claro
    },
  },
  typography: {
    fontFamily: [
      '-apple-system',
      'BlinkMacSystemFont',
      '"Segoe UI"',
      'Roboto',
      '"Helvetica Neue"',
      'Arial',
      'sans-serif',
    ].join(','),
  },
});

function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <ThemeProvider theme={theme}>
        <CssBaseline />
        <BrowserRouter>
          <Routes>
            <Route path="/" element={<Layout />}>
              <Route index element={<DashboardPage />} />
              <Route path="dados" element={<DadosPage />} />
              <Route path="alerts" element={<AlertsPage />} />
              <Route path="consumo" element={<ConsumoPage />} />
              <Route path="mapa" element={<MapaPage />} />
              <Route path="system" element={<SystemPage />} />
            </Route>
          </Routes>
        </BrowserRouter>
      </ThemeProvider>
    </QueryClientProvider>
  );
}

export default App;
