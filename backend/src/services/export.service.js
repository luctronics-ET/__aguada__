import { pool } from '../config/database.js';
import logger from '../config/logger.js';

/**
 * Export data to CSV format
 */
export async function exportToCSV(data, columns) {
  try {
    // Create header row
    const headers = columns.map(col => col.label || col.key).join(',');
    
    // Create data rows
    const rows = data.map(row => {
      return columns.map(col => {
        let value = row[col.key];
        
        // Apply formatter if provided
        if (col.formatter && typeof col.formatter === 'function') {
          value = col.formatter(value);
        }
        
        // Escape commas and quotes
        if (typeof value === 'string') {
          if (value.includes(',') || value.includes('"') || value.includes('\n')) {
            value = `"${value.replace(/"/g, '""')}"`;
          }
        }
        
        return value !== null && value !== undefined ? value : '';
      }).join(',');
    });
    
    return [headers, ...rows].join('\n');
  } catch (error) {
    logger.error('Error exporting to CSV:', error);
    throw error;
  }
}

/**
 * Export readings data
 */
export async function exportReadings(req, res) {
  try {
    const {
      sensor_id,
      start_date,
      end_date,
      format = 'csv'
    } = req.query;

    let query = `
      SELECT 
        l.datetime,
        l.sensor_id,
        s.elemento_id,
        e.nome as elemento_nome,
        l.variavel,
        l.valor,
        l.unidade,
        l.fonte,
        l.modo
      FROM leituras_raw l
      JOIN sensores s ON l.sensor_id = s.sensor_id
      JOIN elementos e ON s.elemento_id = e.elemento_id
      WHERE 1=1
    `;

    const params = [];
    let paramCount = 1;

    if (sensor_id) {
      query += ` AND l.sensor_id = $${paramCount}`;
      params.push(sensor_id);
      paramCount++;
    }

    if (start_date) {
      query += ` AND l.datetime >= $${paramCount}`;
      params.push(start_date);
      paramCount++;
    }

    if (end_date) {
      query += ` AND l.datetime <= $${paramCount}`;
      params.push(end_date);
      paramCount++;
    }

    query += ` ORDER BY l.datetime DESC LIMIT 10000`;

    const result = await pool.query(query, params);

    if (format === 'csv') {
      const columns = [
        { key: 'datetime', label: 'Data/Hora' },
        { key: 'sensor_id', label: 'Sensor' },
        { key: 'elemento_nome', label: 'Elemento' },
        { key: 'variavel', label: 'Variável' },
        { key: 'valor', label: 'Valor' },
        { key: 'unidade', label: 'Unidade' },
        { key: 'fonte', label: 'Fonte' },
        { key: 'modo', label: 'Modo' }
      ];

      const csv = await exportToCSV(result.rows, columns);

      res.setHeader('Content-Type', 'text/csv');
      res.setHeader('Content-Disposition', `attachment; filename=leituras-${new Date().toISOString().split('T')[0]}.csv`);
      res.send(csv);
    } else {
      res.status(400).json({
        success: false,
        error: 'Formato não suportado. Use format=csv'
      });
    }
  } catch (error) {
    logger.error('Error exporting readings:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao exportar leituras'
    });
  }
}

/**
 * Export alerts data
 */
export async function exportAlerts(req, res) {
  try {
    const {
      sensor_id,
      start_date,
      end_date,
      format = 'csv'
    } = req.query;

    let query = `
      SELECT 
        a.datetime,
        a.sensor_id,
        s.elemento_id,
        e.nome as elemento_nome,
        a.tipo,
        a.nivel,
        a.mensagem,
        a.resolvido,
        a.data_resolucao,
        a.observacao
      FROM alertas a
      JOIN sensores s ON a.sensor_id = s.sensor_id
      JOIN elementos e ON s.elemento_id = e.elemento_id
      WHERE 1=1
    `;

    const params = [];
    let paramCount = 1;

    if (sensor_id) {
      query += ` AND a.sensor_id = $${paramCount}`;
      params.push(sensor_id);
      paramCount++;
    }

    if (start_date) {
      query += ` AND a.datetime >= $${paramCount}`;
      params.push(start_date);
      paramCount++;
    }

    if (end_date) {
      query += ` AND a.datetime <= $${paramCount}`;
      params.push(end_date);
      paramCount++;
    }

    query += ` ORDER BY a.datetime DESC`;

    const result = await pool.query(query, params);

    if (format === 'csv') {
      const columns = [
        { key: 'datetime', label: 'Data/Hora' },
        { key: 'sensor_id', label: 'Sensor' },
        { key: 'elemento_nome', label: 'Elemento' },
        { key: 'tipo', label: 'Tipo' },
        { key: 'nivel', label: 'Nível' },
        { key: 'mensagem', label: 'Mensagem' },
        { key: 'resolvido', label: 'Resolvido', formatter: (v) => v ? 'Sim' : 'Não' },
        { key: 'data_resolucao', label: 'Data Resolução' },
        { key: 'observacao', label: 'Observação' }
      ];

      const csv = await exportToCSV(result.rows, columns);

      res.setHeader('Content-Type', 'text/csv');
      res.setHeader('Content-Disposition', `attachment; filename=alertas-${new Date().toISOString().split('T')[0]}.csv`);
      res.send(csv);
    } else {
      res.status(400).json({
        success: false,
        error: 'Formato não suportado. Use format=csv'
      });
    }
  } catch (error) {
    logger.error('Error exporting alerts:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao exportar alertas'
    });
  }
}

export default {
  exportToCSV,
  exportReadings,
  exportAlerts
};
