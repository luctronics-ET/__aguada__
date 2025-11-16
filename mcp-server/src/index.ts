#!/usr/bin/env node

/**
 * AGUADA MCP Server
 * Model Context Protocol server for hydraulic monitoring system
 */

import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  ListResourcesRequestSchema,
  ReadResourceRequestSchema,
} from '@modelcontextprotocol/sdk/types.js';
import * as fs from 'fs/promises';
import * as path from 'path';

// Server configuration
const SERVER_NAME = 'aguada-mcp-server';
const SERVER_VERSION = '1.0.0';

// Project root path (adjust based on deployment)
const PROJECT_ROOT = path.resolve(process.cwd(), '..');

/**
 * AGUADA MCP Server Implementation
 */
class AguadaMCPServer {
  private server: Server;

  constructor() {
    this.server = new Server(
      {
        name: SERVER_NAME,
        version: SERVER_VERSION,
      },
      {
        capabilities: {
          tools: {},
          resources: {},
        },
      }
    );

    this.setupHandlers();
  }

  private setupHandlers() {
    // List available tools
    this.server.setRequestHandler(ListToolsRequestSchema, async () => ({
      tools: [
        {
          name: 'get_telemetry',
          description: 'Retrieve telemetry data from sensors',
          inputSchema: {
            type: 'object',
            properties: {
              node_id: {
                type: 'string',
                description: 'Node ID (e.g., node_10)',
              },
              start_time: {
                type: 'string',
                description: 'Start timestamp (ISO 8601)',
              },
              end_time: {
                type: 'string',
                description: 'End timestamp (ISO 8601)',
              },
            },
            required: ['node_id'],
          },
        },
        {
          name: 'get_reservoir_status',
          description: 'Get current status of a reservoir',
          inputSchema: {
            type: 'object',
            properties: {
              reservoir_id: {
                type: 'string',
                description: 'Reservoir ID (e.g., CAV, CAM, CAS, CI)',
              },
            },
            required: ['reservoir_id'],
          },
        },
        {
          name: 'get_system_overview',
          description: 'Get complete system overview with all reservoirs and sensors',
          inputSchema: {
            type: 'object',
            properties: {},
          },
        },
        {
          name: 'analyze_consumption',
          description: 'Analyze water consumption patterns',
          inputSchema: {
            type: 'object',
            properties: {
              period: {
                type: 'string',
                enum: ['daily', 'weekly', 'monthly'],
                description: 'Analysis period',
              },
            },
            required: ['period'],
          },
        },
        {
          name: 'check_events',
          description: 'Check for hydraulic events (supply, leak, critical level)',
          inputSchema: {
            type: 'object',
            properties: {
              event_type: {
                type: 'string',
                enum: ['abastecimento', 'vazamento', 'nivel_critico', 'all'],
                description: 'Type of event to check',
              },
              hours: {
                type: 'number',
                description: 'Look back hours (default: 24)',
              },
            },
          },
        },
      ],
    }));

    // List available resources
    this.server.setRequestHandler(ListResourcesRequestSchema, async () => ({
      resources: [
        {
          uri: 'aguada://config/reservoirs',
          name: 'Reservoir Configuration',
          mimeType: 'application/json',
          description: 'Configuration of all water reservoirs',
        },
        {
          uri: 'aguada://config/sensors',
          name: 'Sensor Configuration',
          mimeType: 'application/json',
          description: 'Configuration of all sensors',
        },
        {
          uri: 'aguada://config/topology',
          name: 'Network Topology',
          mimeType: 'application/json',
          description: 'Hydraulic network topology',
        },
        {
          uri: 'aguada://docs/schema',
          name: 'Database Schema',
          mimeType: 'text/plain',
          description: 'PostgreSQL database schema',
        },
        {
          uri: 'aguada://docs/api',
          name: 'API Documentation',
          mimeType: 'text/markdown',
          description: 'Backend API documentation',
        },
      ],
    }));

    // Read resource content
    this.server.setRequestHandler(ReadResourceRequestSchema, async (request) => {
      const uri = request.params.uri;

      switch (uri) {
        case 'aguada://config/reservoirs':
          return await this.readConfigFile('reservoirs.json');
        
        case 'aguada://config/sensors':
          return await this.readConfigFile('sensors.json');
        
        case 'aguada://config/topology':
          return await this.readConfigFile('network_topology.json');
        
        case 'aguada://docs/schema':
          return await this.readFile('database/schema.sql', 'text/plain');
        
        case 'aguada://docs/api':
          return await this.readFile('docs/API.md', 'text/markdown');
        
        default:
          throw new Error(`Unknown resource: ${uri}`);
      }
    });

    // Handle tool calls
    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      const { name, arguments: args } = request.params;

      switch (name) {
        case 'get_telemetry':
          return await this.getTelemetry(args as any);
        
        case 'get_reservoir_status':
          return await this.getReservoirStatus(args as any);
        
        case 'get_system_overview':
          return await this.getSystemOverview();
        
        case 'analyze_consumption':
          return await this.analyzeConsumption(args as any);
        
        case 'check_events':
          return await this.checkEvents(args as any);
        
        default:
          throw new Error(`Unknown tool: ${name}`);
      }
    });
  }

  // Helper: Read configuration file
  private async readConfigFile(filename: string) {
    const filePath = path.join(PROJECT_ROOT, 'config', filename);
    const content = await fs.readFile(filePath, 'utf-8');
    return {
      contents: [
        {
          uri: `aguada://config/${filename}`,
          mimeType: 'application/json',
          text: content,
        },
      ],
    };
  }

  // Helper: Read any file
  private async readFile(relativePath: string, mimeType: string) {
    const filePath = path.join(PROJECT_ROOT, relativePath);
    const content = await fs.readFile(filePath, 'utf-8');
    return {
      contents: [
        {
          uri: `aguada://${relativePath}`,
          mimeType,
          text: content,
        },
      ],
    };
  }

  // Tool implementation: Get telemetry
  private async getTelemetry(args: { node_id: string; start_time?: string; end_time?: string }) {
    // In production, this would query the TimescaleDB database
    // For now, return mock data structure
    return {
      content: [
        {
          type: 'text',
          text: JSON.stringify({
            node_id: args.node_id,
            data: 'Mock telemetry data - would query TimescaleDB in production',
            note: 'Connect to PostgreSQL at localhost:5432/aguada_db',
          }, null, 2),
        },
      ],
    };
  }

  // Tool implementation: Get reservoir status
  private async getReservoirStatus(args: { reservoir_id: string }) {
    const configPath = path.join(PROJECT_ROOT, 'config', 'reservoirs.json');
    const config = JSON.parse(await fs.readFile(configPath, 'utf-8'));
    const reservoir = config.reservoirs.find((r: any) => r.id === args.reservoir_id);

    if (!reservoir) {
      throw new Error(`Reservoir not found: ${args.reservoir_id}`);
    }

    return {
      content: [
        {
          type: 'text',
          text: JSON.stringify({
            reservoir,
            status: 'Mock status - would query latest sensor data in production',
          }, null, 2),
        },
      ],
    };
  }

  // Tool implementation: Get system overview
  private async getSystemOverview() {
    const reservoirsPath = path.join(PROJECT_ROOT, 'config', 'reservoirs.json');
    const sensorsPath = path.join(PROJECT_ROOT, 'config', 'sensors.json');
    
    const reservoirs = JSON.parse(await fs.readFile(reservoirsPath, 'utf-8'));
    const sensors = JSON.parse(await fs.readFile(sensorsPath, 'utf-8'));

    return {
      content: [
        {
          type: 'text',
          text: JSON.stringify({
            system: 'AGUADA Hydraulic Monitoring',
            reservoirs: reservoirs.reservoirs,
            sensors: sensors.sensors,
            status: 'All systems operational',
          }, null, 2),
        },
      ],
    };
  }

  // Tool implementation: Analyze consumption
  private async analyzeConsumption(args: { period: string }) {
    return {
      content: [
        {
          type: 'text',
          text: JSON.stringify({
            period: args.period,
            analysis: 'Mock consumption analysis - would query processed data in production',
            note: 'Uses deadband compression algorithm (2cm threshold)',
          }, null, 2),
        },
      ],
    };
  }

  // Tool implementation: Check events
  private async checkEvents(args: { event_type?: string; hours?: number }) {
    return {
      content: [
        {
          type: 'text',
          text: JSON.stringify({
            event_type: args.event_type || 'all',
            lookback_hours: args.hours || 24,
            events: 'Mock events - would query eventos table in production',
            note: 'Events: abastecimento (>50L), vazamento (<-15L/h), nivel_critico (<70%)',
          }, null, 2),
        },
      ],
    };
  }

  async run() {
    const transport = new StdioServerTransport();
    await this.server.connect(transport);
    console.error('AGUADA MCP Server running on stdio');
  }
}

// Start server
const server = new AguadaMCPServer();
server.run().catch((error) => {
  console.error('Server error:', error);
  process.exit(1);
});
