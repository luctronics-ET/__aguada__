import pool from '../config/database.js';
import logger from '../config/logger.js';

/**
 * GET /api/database/tables
 * Lista todas as tabelas do schema aguada
 */
export async function getTables(req, res) {
  try {
    const query = `
      SELECT 
        table_name,
        (SELECT COUNT(*) FROM information_schema.columns 
         WHERE table_schema = 'aguada' AND table_name = t.table_name) as column_count
      FROM information_schema.tables t
      WHERE table_schema = 'aguada'
      ORDER BY table_name;
    `;

    const result = await pool.query(query);
    
    res.json({
      success: true,
      tables: result.rows,
    });
  } catch (error) {
    logger.error('Erro ao listar tabelas:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao listar tabelas',
    });
  }
}

/**
 * GET /api/database/table/:tableName
 * Obtém dados de uma tabela específica com paginação
 */
export async function getTableData(req, res) {
  try {
    const { tableName } = req.params;
    const page = parseInt(req.query.page || '1');
    const limit = parseInt(req.query.limit || '10');
    const offset = (page - 1) * limit;

    // Validar nome da tabela (prevenir SQL injection)
    if (!/^[a-z_][a-z0-9_]*$/i.test(tableName)) {
      return res.status(400).json({
        success: false,
        error: 'Nome de tabela inválido',
      });
    }

    // Verificar se tabela existe
    const tableCheck = await pool.query(`
      SELECT table_name 
      FROM information_schema.tables 
      WHERE table_schema = 'aguada' AND table_name = $1
    `, [tableName]);

    if (tableCheck.rows.length === 0) {
      return res.status(404).json({
        success: false,
        error: 'Tabela não encontrada',
      });
    }

    // Contar total de registros
    const countResult = await pool.query(`SELECT COUNT(*) as total FROM aguada.${tableName}`);
    const total = parseInt(countResult.rows[0].total);

    // Buscar dados com paginação
    const dataResult = await pool.query(`
      SELECT * FROM aguada.${tableName}
      ORDER BY 
        CASE 
          WHEN EXISTS (SELECT 1 FROM information_schema.columns 
                       WHERE table_schema = 'aguada' AND table_name = $1 AND column_name = 'datetime')
          THEN 1
          WHEN EXISTS (SELECT 1 FROM information_schema.columns 
                       WHERE table_schema = 'aguada' AND table_name = $1 AND column_name = 'created_at')
          THEN 2
          ELSE 3
        END
      DESC
      LIMIT $2 OFFSET $3
    `, [tableName, limit, offset]);

    // Obter informações das colunas
    const columnsResult = await pool.query(`
      SELECT 
        column_name,
        data_type,
        is_nullable,
        column_default
      FROM information_schema.columns
      WHERE table_schema = 'aguada' AND table_name = $1
      ORDER BY ordinal_position
    `, [tableName]);

    const totalPages = Math.ceil(total / limit);

    res.json({
      success: true,
      table: tableName,
      columns: columnsResult.rows,
      data: dataResult.rows,
      pagination: {
        page,
        limit,
        total,
        totalPages,
        hasNext: page < totalPages,
        hasPrev: page > 1,
      },
    });
  } catch (error) {
    logger.error('Erro ao buscar dados da tabela:', error);
    res.status(500).json({
      success: false,
      error: 'Erro ao buscar dados da tabela',
      message: error.message,
    });
  }
}

export default {
  getTables,
  getTableData,
};

