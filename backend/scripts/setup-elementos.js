/**
 * Script para configurar tabela elementos no banco de dados
 * Executa via: node backend/scripts/setup-elementos.js
 */

import pg from "pg";
import dotenv from "dotenv";
import { fileURLToPath } from "url";
import { dirname, join } from "path";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Carregar .env do backend
dotenv.config({ path: join(__dirname, "..", ".env") });

const { Pool } = pg;

const pool = new Pool({
  host: process.env.DB_HOST || "localhost",
  port: parseInt(process.env.DB_PORT || "5432"),
  database: process.env.DB_NAME || "aguada",
  user: process.env.DB_USER || "aguada_user",
  password: process.env.DB_PASSWORD,
});

async function setup() {
  const client = await pool.connect();

  try {
    console.log("🔧 Configurando tabela elementos...\n");

    // Verificar se tabela existe
    const tableCheck = await client.query(`
      SELECT EXISTS (
        SELECT FROM information_schema.tables 
        WHERE table_schema = 'aguada' 
        AND table_name = 'elementos'
      );
    `);

    if (!tableCheck.rows[0].exists) {
      console.log("📋 Criando tabela aguada.elementos...");

      await client.query(`
        CREATE TABLE aguada.elementos (
          elemento_id VARCHAR(50) PRIMARY KEY,
          tipo VARCHAR(20) NOT NULL,
          nome VARCHAR(100) NOT NULL,
          descricao TEXT,
          coordenadas JSONB,
          parametros JSONB,
          status VARCHAR(20) DEFAULT 'ativo',
          criado_em TIMESTAMPTZ DEFAULT NOW(),
          atualizado_em TIMESTAMPTZ DEFAULT NOW()
        );
      `);

      console.log("✅ Tabela criada com sucesso!\n");
    } else {
      console.log("✅ Tabela aguada.elementos já existe\n");
    }

    // Inserir dados dos reservatórios
    console.log("📝 Inserindo/atualizando elementos...");

    const elementos = [
      {
        id: "RCON",
        tipo: "reservatorio",
        nome: "Reservatório Concreto",
        descricao: "Reservatório principal de concreto",
        parametros: { altura_cm: 400, diametro_cm: 250, volume_max_l: 19635 },
      },
      {
        id: "RCAV",
        tipo: "reservatorio",
        nome: "Reservatório Caverna",
        descricao: "Reservatório secundário (caverna)",
        parametros: { altura_cm: 180, diametro_cm: 200, volume_max_l: 5655 },
      },
      {
        id: "RB03",
        tipo: "reservatorio",
        nome: "Reservatório Bomba 03",
        descricao: "Reservatório de apoio",
        parametros: { altura_cm: 150, diametro_cm: 100, volume_max_l: 1178 },
      },
      {
        id: "IE01",
        tipo: "reservatorio",
        nome: "Cisterna Ilha 01",
        descricao: "Cisterna subterrânea IE01",
        parametros: { altura_cm: 200, diametro_cm: 180, volume_max_l: 5089 },
      },
      {
        id: "IE02",
        tipo: "reservatorio",
        nome: "Cisterna Ilha 02",
        descricao: "Cisterna subterrânea IE02",
        parametros: { altura_cm: 200, diametro_cm: 180, volume_max_l: 5089 },
      },
    ];

    for (const elem of elementos) {
      await client.query(
        `
        INSERT INTO aguada.elementos (elemento_id, tipo, nome, descricao, parametros, status)
        VALUES ($1, $2, $3, $4, $5, 'ativo')
        ON CONFLICT (elemento_id) DO UPDATE SET
          parametros = EXCLUDED.parametros,
          atualizado_em = NOW()
      `,
        [
          elem.id,
          elem.tipo,
          elem.nome,
          elem.descricao,
          JSON.stringify(elem.parametros),
        ]
      );

      console.log(`  ✓ ${elem.id}: ${elem.nome}`);
    }

    // Atualizar sensores com elemento_id
    console.log("\n📡 Atualizando sensores...");

    const sensorUpdates = [
      { sensor_id: "SEN_CON_01", elemento_id: "RCON" },
      { sensor_id: "SEN_CAV_01", elemento_id: "RCAV" },
    ];

    for (const sensor of sensorUpdates) {
      const result = await client.query(
        `
        UPDATE aguada.sensores 
        SET elemento_id = $2 
        WHERE sensor_id = $1
        RETURNING sensor_id, elemento_id
      `,
        [sensor.sensor_id, sensor.elemento_id]
      );

      if (result.rowCount > 0) {
        console.log(`  ✓ ${sensor.sensor_id} → ${sensor.elemento_id}`);
      } else {
        console.log(`  ⚠ ${sensor.sensor_id} não encontrado`);
      }
    }

    // Verificar resultado
    console.log("\n📊 Verificando dados:");

    const elementos_result = await client.query(`
      SELECT elemento_id, tipo, nome, status 
      FROM aguada.elementos 
      ORDER BY elemento_id
    `);

    console.log("\nElementos:");
    console.table(elementos_result.rows);

    const sensores_result = await client.query(`
      SELECT s.sensor_id, s.elemento_id, s.node_mac, s.variavel, e.nome as elemento_nome
      FROM aguada.sensores s
      LEFT JOIN aguada.elementos e ON s.elemento_id = e.elemento_id
      ORDER BY s.sensor_id
    `);

    console.log("\nSensores:");
    console.table(sensores_result.rows);

    console.log("\n✅ Setup concluído com sucesso!");
  } catch (error) {
    console.error("❌ Erro:", error.message);
    throw error;
  } finally {
    client.release();
    await pool.end();
  }
}

setup().catch(console.error);
