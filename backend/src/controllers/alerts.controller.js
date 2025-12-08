import { pool } from "../config/database.js";
import logger from "../config/logger.js";
import { broadcastAlert } from "../websocket/wsHandler.js";

/**
 * GET /api/alerts
 * Get all alerts with optional filters
 */
export async function getAlerts(req, res) {
  try {
    const {
      sensor_id,
      type,
      level,
      status = "ativo",
      limit = 50,
      offset = 0,
    } = req.query;

    let query = `
      SELECT 
        a.alerta_id,
        a.tipo,
        a.elemento_id,
        a.sensor_id,
        a.descricao,
        a.nivel,
        a.status,
        a.datetime_criacao,
        a.datetime_resolucao,
        e.nome as elemento_nome
      FROM aguada.alertas a
      LEFT JOIN aguada.elementos e ON a.elemento_id = e.elemento_id
      WHERE 1=1
    `;

    const params = [];
    let paramCount = 1;

    if (sensor_id) {
      query += ` AND a.sensor_id = $${paramCount}`;
      params.push(sensor_id);
      paramCount++;
    }

    if (type) {
      query += ` AND a.tipo = $${paramCount}`;
      params.push(type);
      paramCount++;
    }

    if (level) {
      query += ` AND a.nivel = $${paramCount}`;
      params.push(level);
      paramCount++;
    }

    if (status === "ativo" || status === "active") {
      query += ` AND a.status = 'ativo'`;
    } else if (status === "resolvido" || status === "resolved") {
      query += ` AND a.status = 'resolvido'`;
    }

    query += ` ORDER BY a.datetime_criacao DESC LIMIT $${paramCount} OFFSET $${
      paramCount + 1
    }`;
    params.push(parseInt(limit), parseInt(offset));

    const result = await pool.query(query, params);

    // Get total count
    let countQuery = `SELECT COUNT(*) as total FROM aguada.alertas WHERE 1=1`;
    const countParams = [];

    if (status === "ativo" || status === "active") {
      countQuery += ` AND status = 'ativo'`;
    } else if (status === "resolvido" || status === "resolved") {
      countQuery += ` AND status = 'resolvido'`;
    }

    const countResult = await pool.query(countQuery, countParams);

    res.status(200).json({
      success: true,
      count: result.rows.length,
      total: parseInt(countResult.rows[0].total),
      data: result.rows,
    });
  } catch (error) {
    logger.error("Error fetching alerts:", error);
    logger.error("Error details:", {
      message: error.message,
      stack: error.stack,
      code: error.code,
    });
    res.status(500).json({
      success: false,
      error: "Erro ao buscar alertas",
      details:
        process.env.NODE_ENV === "development" ? error.message : undefined,
    });
  }
}

/**
 * POST /api/alerts
 * Create a new alert
 */
export async function createAlert(req, res) {
  try {
    const { sensor_id, tipo, nivel, mensagem, detalhes } = req.body;

    if (!sensor_id || !tipo || !nivel || !mensagem) {
      return res.status(400).json({
        success: false,
        error: "Campos obrigatórios: sensor_id, tipo, nivel, mensagem",
      });
    }

    const result = await pool.query(
      `
      INSERT INTO aguada.alertas (sensor_id, tipo, nivel, mensagem, detalhes, datetime)
      VALUES ($1, $2, $3, $4, $5, NOW())
      RETURNING *
    `,
      [sensor_id, tipo, nivel, mensagem, detalhes || null]
    );

    const alert = result.rows[0];

    // Broadcast alert via WebSocket
    broadcastAlert(alert);

    logger.info(`Alert created: ${tipo} - ${nivel}`, { sensor_id, mensagem });

    res.status(201).json({
      success: true,
      message: "Alerta criado com sucesso",
      data: alert,
    });
  } catch (error) {
    logger.error("Error creating alert:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao criar alerta",
    });
  }
}

/**
 * PUT /api/alerts/:alert_id/resolve
 * Mark alert as resolved
 */
export async function resolveAlert(req, res) {
  try {
    const { alert_id } = req.params;
    const { observacao } = req.body;

    const result = await pool.query(
      `
      UPDATE aguada.alertas 
      SET status = 'resolvido',
          datetime_resolucao = NOW(),
          observacao = $2
      WHERE alerta_id = $1
      RETURNING *
    `,
      [alert_id, observacao || null]
    );

    if (result.rows.length === 0) {
      return res.status(404).json({
        success: false,
        error: "Alerta não encontrado",
      });
    }

    logger.info(`Alert resolved: ${alert_id}`);

    res.status(200).json({
      success: true,
      message: "Alerta resolvido com sucesso",
      data: result.rows[0],
    });
  } catch (error) {
    logger.error("Error resolving alert:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao resolver alerta",
    });
  }
}

/**
 * GET /api/alerts/summary
 * Get alerts summary
 */
export async function getAlertsSummary(req, res) {
  try {
    const result = await pool.query(`
      SELECT 
        nivel,
        COUNT(*) as count,
        COUNT(CASE WHEN status = 'ativo' THEN 1 END) as active,
        COUNT(CASE WHEN status = 'resolvido' THEN 1 END) as resolved
      FROM aguada.alertas
      WHERE datetime_criacao > NOW() - INTERVAL '7 days'
      GROUP BY nivel
    `);

    const summary = {
      total: 0,
      active: 0,
      resolved: 0,
      by_level: {},
    };

    result.rows.forEach((row) => {
      summary.total += parseInt(row.count);
      summary.active += parseInt(row.active);
      summary.resolved += parseInt(row.resolved);
      summary.by_level[row.nivel] = {
        count: parseInt(row.count),
        active: parseInt(row.active),
        resolved: parseInt(row.resolved),
      };
    });

    res.status(200).json({
      success: true,
      data: summary,
    });
  } catch (error) {
    logger.error("Error fetching alerts summary:", error);
    res.status(500).json({
      success: false,
      error: "Erro ao buscar resumo de alertas",
    });
  }
}

export default {
  getAlerts,
  createAlert,
  resolveAlert,
  getAlertsSummary,
};
