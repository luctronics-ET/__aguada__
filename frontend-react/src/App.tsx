import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ThemeProvider, createTheme, CssBaseline } from '@mui/material';
import DashboardPage from './pages/DashboardPage';

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
        <DashboardPage />
      </ThemeProvider>
    </QueryClientProvider>
  );
}

export default App;
