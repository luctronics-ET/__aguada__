# AGUADA MCP Server

Model Context Protocol server for the AGUADA Hydraulic Monitoring System.

## Overview

This MCP server provides programmatic access to AGUADA's hydraulic monitoring data and configuration through a standardized protocol.

## Features

### Tools
- `get_telemetry` - Retrieve sensor telemetry data
- `get_reservoir_status` - Get current reservoir status
- `get_system_overview` - Complete system overview
- `analyze_consumption` - Analyze water consumption patterns
- `check_events` - Check for hydraulic events

### Resources
- `aguada://config/reservoirs` - Reservoir configuration
- `aguada://config/sensors` - Sensor configuration
- `aguada://config/topology` - Network topology
- `aguada://docs/schema` - Database schema
- `aguada://docs/api` - API documentation

## Installation

```bash
cd mcp-server
npm install
npm run build
```

## Usage

### Development
```bash
npm run dev    # Watch mode with TypeScript
npm start      # Run compiled server
```

### MCP Inspector
```bash
npm run inspector
```

### VS Code Integration

Add to your VS Code settings (`.vscode/settings.json`):

```json
{
  "mcp.servers": {
    "aguada": {
      "command": "node",
      "args": ["/path/to/aguada/mcp-server/dist/index.js"],
      "env": {}
    }
  }
}
```

## Architecture

```
┌─────────────────┐
│  MCP Client     │ (VS Code, Claude Desktop, etc.)
│  (Your IDE)     │
└────────┬────────┘
         │ stdio
         │
┌────────▼────────┐
│  AGUADA MCP     │
│  Server         │
├─────────────────┤
│ - Tools         │
│ - Resources     │
│ - Config Access │
└────────┬────────┘
         │
         │ File System / Database
         │
┌────────▼────────┐
│  AGUADA System  │
├─────────────────┤
│ - Config Files  │
│ - Database      │
│ - Docs          │
└─────────────────┘
```

## Configuration

The server reads from the parent directory structure:
- `/config/*.json` - Configuration files
- `/database/schema.sql` - Database schema
- `/docs/*.md` - Documentation

## Development

### Project Structure
```
mcp-server/
├── src/
│   └── index.ts          # Main server implementation
├── dist/                 # Compiled JavaScript
├── package.json
├── tsconfig.json
└── README.md
```

### Adding New Tools

1. Add tool definition in `ListToolsRequestSchema` handler
2. Implement tool function
3. Add case in `CallToolRequestSchema` handler

### Adding New Resources

1. Add resource definition in `ListResourcesRequestSchema` handler
2. Add case in `ReadResourceRequestSchema` handler

## Production Deployment

For production, the server should:
1. Connect to PostgreSQL/TimescaleDB instead of mock data
2. Add authentication/authorization
3. Implement rate limiting
4. Add comprehensive error handling
5. Enable logging and monitoring

## License

MIT - Part of the AGUADA Hydraulic Monitoring System
