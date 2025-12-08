#!/usr/bin/env python3
"""
AGUADA Monitor Server
Servidor web que lê dados do gateway serial e serve via WebSocket/HTTP

Uso:
    python monitor_server.py --port /dev/ttyACM1
    
Depois acesse: http://localhost:8080
"""

import asyncio
import json
import argparse
import serial
import threading
from datetime import datetime
from http.server import HTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import webbrowser

# Armazenamento de leituras
readings = []
MAX_READINGS = 1000
reading_id = 0
connected_clients = []

# HTML da página
HTML_PAGE = '''<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AGUADA - Monitor de Leituras</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: #fff;
            padding: 20px;
        }
        .container { max-width: 1400px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        h1 {
            font-size: 2.5em;
            background: linear-gradient(90deg, #00d4ff, #00ff88);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 10px;
        }
        .status {
            display: flex;
            justify-content: center;
            gap: 30px;
            flex-wrap: wrap;
            margin-top: 15px;
        }
        .status-item {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 8px 15px;
            background: rgba(0,0,0,0.3);
            border-radius: 20px;
        }
        .status-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        .status-dot.connected { background: #00ff88; }
        .status-dot.disconnected { background: #ff4444; }
        @keyframes pulse {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.5; transform: scale(1.1); }
        }
        .controls {
            display: flex;
            gap: 15px;
            justify-content: center;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }
        button {
            padding: 12px 25px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            font-weight: 600;
            transition: all 0.3s ease;
            background: rgba(255,255,255,0.1);
            color: #fff;
            border: 1px solid rgba(255,255,255,0.2);
        }
        button:hover {
            background: rgba(255,255,255,0.2);
            transform: translateY(-2px);
        }
        .stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 15px;
            margin-bottom: 25px;
        }
        .stat-card {
            background: rgba(255,255,255,0.05);
            border-radius: 12px;
            padding: 20px;
            text-align: center;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .stat-value {
            font-size: 2em;
            font-weight: bold;
            color: #00d4ff;
        }
        .stat-label { color: #888; margin-top: 5px; }
        .table-container {
            background: rgba(255,255,255,0.05);
            border-radius: 15px;
            overflow: hidden;
            border: 1px solid rgba(255,255,255,0.1);
        }
        table { width: 100%; border-collapse: collapse; }
        th, td {
            padding: 12px 15px;
            text-align: left;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        th {
            background: rgba(0,212,255,0.1);
            color: #00d4ff;
            font-weight: 600;
            text-transform: uppercase;
            font-size: 0.85em;
            letter-spacing: 1px;
            position: sticky;
            top: 0;
        }
        tr:hover { background: rgba(255,255,255,0.03); }
        .mac-address {
            font-family: 'Courier New', monospace;
            color: #00ff88;
            font-size: 0.9em;
        }
        .distance { font-weight: bold; color: #fff; font-size: 1.1em; }
        .rssi { display: inline-flex; align-items: center; gap: 5px; }
        .rssi-bar {
            width: 50px;
            height: 8px;
            background: rgba(255,255,255,0.1);
            border-radius: 4px;
            overflow: hidden;
        }
        .rssi-fill { height: 100%; border-radius: 4px; }
        .rssi-excellent { background: #00ff88; }
        .rssi-good { background: #88ff00; }
        .rssi-fair { background: #ffcc00; }
        .rssi-poor { background: #ff4444; }
        .datetime { color: #888; font-size: 0.9em; }
        .empty-state {
            text-align: center;
            padding: 60px 20px;
            color: #666;
        }
        .table-scroll {
            max-height: 500px;
            overflow-y: auto;
        }
        @media (max-width: 768px) {
            h1 { font-size: 1.8em; }
            th, td { padding: 8px 6px; font-size: 0.85em; }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>💧 AGUADA Monitor</h1>
            <p>Sistema de Monitoramento de Reservatórios - Tempo Real</p>
            <div class="status">
                <div class="status-item">
                    <div class="status-dot" id="wsStatus"></div>
                    <span id="wsStatusText">Conectando...</span>
                </div>
                <div class="status-item">
                    <span>📡 Canal: <strong>11</strong></span>
                </div>
                <div class="status-item">
                    <span id="lastUpdate">Última: --</span>
                </div>
            </div>
        </header>
        
        <div class="controls">
            <button onclick="clearReadings()">🗑️ Limpar</button>
            <button onclick="exportCSV()">📥 Exportar CSV</button>
            <button onclick="togglePause()">
                <span id="pauseIcon">⏸️</span> <span id="pauseText">Pausar</span>
            </button>
        </div>
        
        <div class="stats">
            <div class="stat-card">
                <div class="stat-value" id="totalReadings">0</div>
                <div class="stat-label">Total Leituras</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="lastDistance">--</div>
                <div class="stat-label">Distância (cm)</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="avgRssi">--</div>
                <div class="stat-label">RSSI Médio</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="uptime">--</div>
                <div class="stat-label">Uptime (s)</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="packetsPerSec">0</div>
                <div class="stat-label">Pacotes/min</div>
            </div>
        </div>
        
        <div class="table-container">
            <div class="table-scroll">
                <table>
                    <thead>
                        <tr>
                            <th>#</th>
                            <th>Data/Hora</th>
                            <th>MAC</th>
                            <th>Distância (cm)</th>
                            <th>RSSI</th>
                            <th>Uptime</th>
                        </tr>
                    </thead>
                    <tbody id="readingsTable">
                        <tr><td colspan="6" class="empty-state">Aguardando leituras...</td></tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>

    <script>
        let readings = [];
        let paused = false;
        let ws = null;
        let packetTimes = [];
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.host + '/ws');
            
            ws.onopen = () => {
                document.getElementById('wsStatus').className = 'status-dot connected';
                document.getElementById('wsStatusText').textContent = 'Conectado';
            };
            
            ws.onclose = () => {
                document.getElementById('wsStatus').className = 'status-dot disconnected';
                document.getElementById('wsStatusText').textContent = 'Desconectado';
                setTimeout(connect, 2000);
            };
            
            ws.onmessage = (event) => {
                if (paused) return;
                try {
                    const data = JSON.parse(event.data);
                    if (data.type === 'reading') {
                        addReading(data.data);
                    } else if (data.type === 'history') {
                        data.data.forEach(r => addReading(r, false));
                        updateTable();
                    }
                } catch (e) {}
            };
        }
        
        function addReading(data, update = true) {
            if (data.type === 'gateway_boot' || data.type === 'gateway_status') return;
            
            readings.unshift(data);
            if (readings.length > 500) readings.pop();
            
            packetTimes.push(Date.now());
            packetTimes = packetTimes.filter(t => Date.now() - t < 60000);
            
            if (update) updateTable();
        }
        
        function updateTable() {
            const tbody = document.getElementById('readingsTable');
            
            if (readings.length === 0) {
                tbody.innerHTML = '<tr><td colspan="6" class="empty-state">Aguardando leituras...</td></tr>';
                return;
            }
            
            tbody.innerHTML = readings.slice(0, 100).map((r, i) => {
                const dist = (r.value / 100).toFixed(2);
                const rssiClass = r.rssi >= -40 ? 'rssi-excellent' : r.rssi >= -55 ? 'rssi-good' : r.rssi >= -70 ? 'rssi-fair' : 'rssi-poor';
                const rssiPercent = Math.min(100, Math.max(0, (r.rssi + 100) * 2));
                
                return '<tr>' +
                    '<td>' + r.id + '</td>' +
                    '<td class="datetime">' + r.datetime + '</td>' +
                    '<td class="mac-address">' + r.mac + '</td>' +
                    '<td class="distance">' + dist + '</td>' +
                    '<td><span class="rssi"><div class="rssi-bar"><div class="rssi-fill ' + rssiClass + '" style="width:' + rssiPercent + '%"></div></div>' + r.rssi + ' dBm</span></td>' +
                    '<td>' + r.uptime + '</td>' +
                '</tr>';
            }).join('');
            
            // Stats
            document.getElementById('totalReadings').textContent = readings.length;
            if (readings.length > 0) {
                document.getElementById('lastDistance').textContent = (readings[0].value / 100).toFixed(2);
                document.getElementById('uptime').textContent = readings[0].uptime;
                document.getElementById('lastUpdate').textContent = 'Última: ' + readings[0].datetime.split(' ')[1];
                
                const avgRssi = readings.slice(0, 10).reduce((s, r) => s + r.rssi, 0) / Math.min(10, readings.length);
                document.getElementById('avgRssi').textContent = avgRssi.toFixed(0);
            }
            document.getElementById('packetsPerSec').textContent = packetTimes.length;
        }
        
        function clearReadings() {
            readings = [];
            updateTable();
        }
        
        function togglePause() {
            paused = !paused;
            document.getElementById('pauseIcon').textContent = paused ? '▶️' : '⏸️';
            document.getElementById('pauseText').textContent = paused ? 'Continuar' : 'Pausar';
        }
        
        function exportCSV() {
            if (readings.length === 0) { alert('Sem dados'); return; }
            const csv = 'ID,DateTime,MAC,Type,Value,Distance_cm,RSSI,Uptime\\n' +
                readings.map(r => [r.id, r.datetime, r.mac, r.type, r.value, (r.value/100).toFixed(2), r.rssi, r.uptime].join(',')).join('\\n');
            const blob = new Blob([csv], {type: 'text/csv'});
            const a = document.createElement('a');
            a.href = URL.createObjectURL(blob);
            a.download = 'aguada_' + new Date().toISOString().slice(0,10) + '.csv';
            a.click();
        }
        
        connect();
        setInterval(updateTable, 1000);
    </script>
</body>
</html>
'''

class WebSocketHandler:
    def __init__(self):
        self.clients = []
    
    def add_client(self, writer):
        self.clients.append(writer)
    
    def remove_client(self, writer):
        if writer in self.clients:
            self.clients.remove(writer)
    
    async def broadcast(self, message):
        for client in self.clients[:]:
            try:
                await client.send(message)
            except:
                self.remove_client(client)

ws_handler = WebSocketHandler()

def serial_reader(port, baudrate=115200):
    """Thread que lê a porta serial"""
    global readings, reading_id
    
    print(f"📡 Conectando à porta serial {port}...")
    
    while True:
        try:
            ser = serial.Serial(port, baudrate, timeout=1)
            print(f"✅ Conectado a {port}")
            
            while True:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    if line.startswith('{'):
                        try:
                            data = json.loads(line)
                            
                            # Ignorar mensagens do gateway
                            if data.get('type') in ['gateway_boot', 'gateway_status']:
                                continue
                            
                            reading_id += 1
                            data['id'] = reading_id
                            data['datetime'] = datetime.now().strftime('%d/%m/%Y %H:%M:%S')
                            
                            readings.insert(0, data)
                            if len(readings) > MAX_READINGS:
                                readings.pop()
                            
                            print(f"📦 [{reading_id}] {data.get('mac', '?')} → {data.get('value', 0)/100:.2f} cm (RSSI: {data.get('rssi', 0)} dBm)")
                            
                        except json.JSONDecodeError:
                            pass
                            
        except serial.SerialException as e:
            print(f"❌ Erro serial: {e}")
            import time
            time.sleep(2)
        except Exception as e:
            print(f"❌ Erro: {e}")
            import time
            time.sleep(2)

async def handle_websocket(websocket, path):
    """Handler WebSocket"""
    print(f"🔌 Cliente WebSocket conectado")
    
    # Enviar histórico
    if readings:
        await websocket.send(json.dumps({'type': 'history', 'data': readings[:100]}))
    
    ws_handler.add_client(websocket)
    
    last_id = reading_id
    
    try:
        while True:
            # Verificar novas leituras
            if readings and readings[0]['id'] > last_id:
                for r in readings:
                    if r['id'] <= last_id:
                        break
                    await websocket.send(json.dumps({'type': 'reading', 'data': r}))
                last_id = readings[0]['id']
            
            await asyncio.sleep(0.1)
            
    except Exception as e:
        pass
    finally:
        ws_handler.remove_client(websocket)
        print(f"🔌 Cliente WebSocket desconectado")

def run_http_server(port=8080):
    """Servidor HTTP simples"""
    class Handler(SimpleHTTPRequestHandler):
        def do_GET(self):
            if self.path == '/' or self.path == '/index.html':
                self.send_response(200)
                self.send_header('Content-type', 'text/html; charset=utf-8')
                self.end_headers()
                self.wfile.write(HTML_PAGE.encode('utf-8'))
            elif self.path == '/api/readings':
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps(readings[:100]).encode('utf-8'))
            else:
                self.send_error(404)
        
        def log_message(self, format, *args):
            pass  # Silenciar logs HTTP
    
    server = HTTPServer(('0.0.0.0', port), Handler)
    print(f"🌐 Servidor HTTP em http://localhost:{port}")
    server.serve_forever()

async def main(serial_port, http_port=8080, ws_port=8081):
    """Main async"""
    import websockets
    
    # Thread serial
    serial_thread = threading.Thread(target=serial_reader, args=(serial_port,), daemon=True)
    serial_thread.start()
    
    # Thread HTTP
    http_thread = threading.Thread(target=run_http_server, args=(http_port,), daemon=True)
    http_thread.start()
    
    print(f"🔌 WebSocket em ws://localhost:{ws_port}")
    print(f"\n{'='*50}")
    print(f"  Abra no navegador: http://localhost:{http_port}")
    print(f"{'='*50}\n")
    
    # Abrir navegador
    import time
    time.sleep(1)
    webbrowser.open(f'http://localhost:{http_port}')
    
    # WebSocket server
    async with websockets.serve(handle_websocket, '0.0.0.0', ws_port):
        await asyncio.Future()  # Rodar forever

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='AGUADA Monitor Server')
    parser.add_argument('--port', '-p', default='/dev/ttyACM1', help='Porta serial do gateway')
    parser.add_argument('--http', default=8080, type=int, help='Porta HTTP')
    parser.add_argument('--ws', default=8081, type=int, help='Porta WebSocket')
    args = parser.parse_args()
    
    print(f"""
╔══════════════════════════════════════════════════╗
║       💧 AGUADA Monitor Server v1.0              ║
╠══════════════════════════════════════════════════╣
║  Serial: {args.port:<39} ║
║  HTTP:   http://localhost:{args.http:<21} ║
║  WS:     ws://localhost:{args.ws:<23} ║
╚══════════════════════════════════════════════════╝
""")
    
    try:
        asyncio.run(main(args.port, args.http, args.ws))
    except KeyboardInterrupt:
        print("\n👋 Encerrado")
