import axios from 'axios';

const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || 'http://192.168.0.100:3000/api',
  timeout: 5000,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Interceptor para tratamento de erros
api.interceptors.response.use(
  (response) => response,
  (error) => {
    console.error('API Error:', error);
    
    if (error.response) {
      // O servidor respondeu com um status de erro
      console.error('Response error:', error.response.status, error.response.data);
    } else if (error.request) {
      // A requisição foi feita mas não houve resposta
      console.error('No response received:', error.request);
    } else {
      // Erro na configuração da requisição
      console.error('Request error:', error.message);
    }
    
    return Promise.reject(error);
  }
);

export default api;
