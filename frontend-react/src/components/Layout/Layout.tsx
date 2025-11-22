import { Outlet } from 'react-router-dom';
import { Box, Container } from '@mui/material';
import Navbar from './Navbar';

export default function Layout() {
  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', minHeight: '100vh' }}>
      <Navbar />
      <Container 
        component="main" 
        maxWidth="xl" 
        sx={{ 
          flexGrow: 1, 
          py: 3,
          px: { xs: 2, sm: 3 }
        }}
      >
        <Outlet />
      </Container>
    </Box>
  );
}
