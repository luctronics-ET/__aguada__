/**
 * AGUADA - Firmware Controller
 * Gerenciamento de firmware OTA para gateways e nodes
 */

import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import logger from "../config/logger.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Diretório para armazenar firmwares
const FIRMWARE_DIR = path.join(__dirname, "../../firmware");

// Versões atuais dos firmwares (em produção, isso viria do banco de dados)
const firmwareVersions = {
  OTA_01: {
    version: "1.0.0",
    filename: "gateway_ota_01.bin",
    description: "Gateway OTA v1",
  },
  SENSOR_10: {
    version: "1.0.0",
    filename: "node_sensor_10.bin",
    description: "Node Sensor Single",
  },
  SENSOR_20: {
    version: "1.0.0",
    filename: "node_sensor_20.bin",
    description: "Node Sensor Dual",
  },
};

// Registro de gateways conhecidos
const knownGateways = new Map();

/**
 * Verificar se há atualização de firmware disponível
 * GET /api/firmware/gateway/check
 * Query: { mac, version, type }
 * Response: 200 (nova versão disponível) | 204 (atualizado)
 */
const checkFirmwareUpdate = async (req, res) => {
  try {
    const { mac, version, type } = req.query;

    if (!mac || !version || !type) {
      return res.status(400).json({
        error: "Parâmetros obrigatórios: mac, version, type",
      });
    }

    // Registrar gateway
    knownGateways.set(mac, {
      type,
      currentVersion: version,
      lastCheck: new Date().toISOString(),
      ip: req.ip || req.connection?.remoteAddress,
    });

    logger.info(
      `[Firmware] Check OTA: MAC=${mac}, type=${type}, version=${version}`
    );

    // Verificar se há nova versão
    const firmwareInfo = firmwareVersions[type];

    if (!firmwareInfo) {
      logger.warn(`[Firmware] Tipo de firmware desconhecido: ${type}`);
      return res.status(404).json({
        error: `Tipo de firmware desconhecido: ${type}`,
      });
    }

    // Comparar versões (simples: string comparison)
    // Em produção, usar semver para comparação correta
    if (firmwareInfo.version !== version) {
      // Nova versão disponível
      logger.info(
        `[Firmware] Nova versão disponível para ${mac}: ${version} → ${firmwareInfo.version}`
      );
      return res.status(200).json({
        update_available: true,
        current_version: version,
        new_version: firmwareInfo.version,
        description: firmwareInfo.description,
        download_url: `/api/firmware/gateway/download?type=${type}`,
      });
    }

    // Firmware está atualizado
    logger.info(`[Firmware] Gateway ${mac} já está atualizado (v${version})`);
    return res.status(204).send();
  } catch (error) {
    logger.error("[Firmware] Erro ao verificar atualização:", error);
    return res.status(500).json({
      error: "Erro interno ao verificar atualização",
    });
  }
};

/**
 * Download do firmware
 * GET /api/firmware/gateway/download
 * Query: { type }
 * Response: application/octet-stream (firmware.bin)
 */
const downloadFirmware = async (req, res) => {
  try {
    const { type } = req.query;

    if (!type) {
      return res.status(400).json({
        error: "Parâmetro obrigatório: type",
      });
    }

    const firmwareInfo = firmwareVersions[type];

    if (!firmwareInfo) {
      return res.status(404).json({
        error: `Tipo de firmware desconhecido: ${type}`,
      });
    }

    const firmwarePath = path.join(FIRMWARE_DIR, firmwareInfo.filename);

    // Verificar se arquivo existe
    if (!fs.existsSync(firmwarePath)) {
      logger.warn(`[Firmware] Arquivo não encontrado: ${firmwarePath}`);
      return res.status(404).json({
        error: "Arquivo de firmware não encontrado",
        hint: `Copie o arquivo ${firmwareInfo.filename} para ${FIRMWARE_DIR}`,
      });
    }

    const stats = fs.statSync(firmwarePath);

    logger.info(
      `[Firmware] Download solicitado: ${firmwareInfo.filename} (${stats.size} bytes)`
    );

    // Enviar arquivo
    res.setHeader("Content-Type", "application/octet-stream");
    res.setHeader(
      "Content-Disposition",
      `attachment; filename="${firmwareInfo.filename}"`
    );
    res.setHeader("Content-Length", stats.size);
    res.setHeader("X-Firmware-Version", firmwareInfo.version);

    const readStream = fs.createReadStream(firmwarePath);
    readStream.pipe(res);
  } catch (error) {
    logger.error("[Firmware] Erro ao fazer download:", error);
    return res.status(500).json({
      error: "Erro interno ao fazer download do firmware",
    });
  }
};

/**
 * Upload de novo firmware (admin)
 * POST /api/firmware/upload
 * Body: multipart/form-data { file, type, version }
 */
const uploadFirmware = async (req, res) => {
  try {
    // TODO: Implementar upload com multer
    // Por enquanto, apenas placeholder
    return res.status(501).json({
      error: "Upload de firmware ainda não implementado",
      hint: "Copie o arquivo .bin manualmente para backend/firmware/",
    });
  } catch (error) {
    logger.error("[Firmware] Erro ao fazer upload:", error);
    return res.status(500).json({
      error: "Erro interno ao fazer upload do firmware",
    });
  }
};

/**
 * Listar versões de firmware disponíveis
 * GET /api/firmware/versions
 */
const listFirmwareVersions = async (req, res) => {
  try {
    const versions = Object.entries(firmwareVersions).map(([type, info]) => {
      const firmwarePath = path.join(FIRMWARE_DIR, info.filename);
      const exists = fs.existsSync(firmwarePath);

      return {
        type,
        version: info.version,
        filename: info.filename,
        description: info.description,
        file_exists: exists,
        file_size: exists ? fs.statSync(firmwarePath).size : null,
      };
    });

    return res.json({
      firmware_directory: FIRMWARE_DIR,
      versions,
    });
  } catch (error) {
    logger.error("[Firmware] Erro ao listar versões:", error);
    return res.status(500).json({
      error: "Erro interno ao listar versões de firmware",
    });
  }
};

/**
 * Listar gateways conhecidos (que fizeram check de OTA)
 * GET /api/firmware/gateways
 */
const listKnownGateways = async (req, res) => {
  try {
    const gateways = Array.from(knownGateways.entries()).map(([mac, info]) => ({
      mac,
      ...info,
    }));

    return res.json({
      count: gateways.length,
      gateways,
    });
  } catch (error) {
    logger.error("[Firmware] Erro ao listar gateways:", error);
    return res.status(500).json({
      error: "Erro interno ao listar gateways",
    });
  }
};

/**
 * Atualizar versão de firmware (para forçar atualização)
 * PUT /api/firmware/version
 * Body: { type, version }
 */
const updateFirmwareVersion = async (req, res) => {
  try {
    const { type, version } = req.body;

    if (!type || !version) {
      return res.status(400).json({
        error: "Parâmetros obrigatórios: type, version",
      });
    }

    if (!firmwareVersions[type]) {
      return res.status(404).json({
        error: `Tipo de firmware desconhecido: ${type}`,
      });
    }

    const oldVersion = firmwareVersions[type].version;
    firmwareVersions[type].version = version;

    logger.info(
      `[Firmware] Versão atualizada: ${type} ${oldVersion} → ${version}`
    );

    return res.json({
      success: true,
      type,
      old_version: oldVersion,
      new_version: version,
      message: "Gateways farão download na próxima verificação",
    });
  } catch (error) {
    logger.error("[Firmware] Erro ao atualizar versão:", error);
    return res.status(500).json({
      error: "Erro interno ao atualizar versão de firmware",
    });
  }
};

export default {
  checkFirmwareUpdate,
  downloadFirmware,
  uploadFirmware,
  listFirmwareVersions,
  listKnownGateways,
  updateFirmwareVersion,
};
