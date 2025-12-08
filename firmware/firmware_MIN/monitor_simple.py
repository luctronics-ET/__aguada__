#!/usr/bin/env python3
"""
AGUADA Monitor Simples - Servidor HTTP que lê gateway e mostra leituras
"""

import json
import serial
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime
import webbrowser
import sys

# Configuração
SERIAL_PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM1'
HTTP_PORT = 8080

# Dados
readings = []
reading_id = 0

HTML = '''<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>AGUADA Monitor</title>
    <meta http-equiv="refresh" content="2">
    <style>
        body { font-family: Arial; background: #1a1a2e; color: #fff; padding: 20px; margin: 0; }
        h1 { color: #00d4ff; text-align: center; }
        .status { text-align: center; margin: 20px 0; color: #00ff88; }
        table { width: 100%; border-collapse: collapse; background: rgba(255,255,255,0.05); }
        th, td { padding: 10px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); }
        th { background: rgba(0,212,255,0.2); color: #00d4ff; }
        .mac { font-family: monospace; color: #00ff88; }
        .dist { font-weight: bold; font-size: 1.2em; }
        .stats { display: flex; gap: 20px; justify-content: center; margin: 20px 0; }
        .stat { background: rgba(255,255,255,0.05); padding: 15px 25px; border-radius: 10px; text-align: center; }
        .stat-val { font-size: 2em; color: #00d4ff; }
        .stat-label { color: #888; }
    </style>
</head>
<body>
    <h1>💧 AGUADA Monitor</h1>
    <p class="status">Gateway: ''' + SERIAL_PORT + ''' | Atualização automática a cada 2s</p>
    
    <div class="stats">
        <div class="stat">
            <div class="stat-val" id="total">''' + str(len(readings)) + '''</div>
            <div class="stat-label">Leituras</div>
        </div>
        <div class="stat">
            <div class="stat-val" id="last">''' + (str(readings[0]['value']/100) if readings else '--') + '''</div>
            <div class="stat-label">Última (cm)</div>
        </div>
    </div>
    
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
        <tbody>
            %%%ROWS%%%
        </tbody>
    </table>
</body>
</html>
'''

def generate_rows():
    rows = []
    for r in readings[:100]:
        rows.append(f'''<tr>
            <td>{r['id']}</td>
            <td>{r['datetime']}</td>
            <td class="mac">{r['mac']}</td>
            <td class="dist">{r['value']/100:.2f}</td>
            <td>{r['rssi']} dBm</td>
            <td>{r['uptime']}s</td>
        </tr>''')
    return '\n'.join(rows) if rows else '<tr><td colspan="6" style="text-align:center;padding:40px;color:#666;">Aguardando leituras...</td></tr>'

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            html = HTML.replace('%%%ROWS%%%', generate_rows())
            html = html.replace("str(len(readings))", str(len(readings)))
            self.send_response(200)
            self.send_header('Content-type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(html.encode())
        elif self.path == '/api/readings':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(readings[:100]).encode())
        else:
            self.send_error(404)
    
    def log_message(self, format, *args):
        pass

def serial_thread():
    global readings, reading_id
    
    while True:
        try:
            print(f"📡 Conectando a {SERIAL_PORT}...")
            ser = serial.Serial(SERIAL_PORT, 115200, timeout=1)
            print(f"✅ Conectado!")
            
            while True:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='replace').strip()
                    if line.startswith('{'):
                        try:
                            data = json.loads(line)
                            if data.get('type') in ['gateway_boot', 'gateway_status']:
                                continue
                            
                            reading_id += 1
                            data['id'] = reading_id
                            data['datetime'] = datetime.now().strftime('%d/%m/%Y %H:%M:%S')
                            
                            readings.insert(0, data)
                            if len(readings) > 500:
                                readings.pop()
                            
                            print(f"📦 [{reading_id}] {data.get('value',0)/100:.2f} cm (RSSI: {data.get('rssi',0)})")
                        except:
                            pass
        except Exception as e:
            print(f"❌ Erro: {e}")
            time.sleep(2)

if __name__ == '__main__':
    print(f"""
╔══════════════════════════════════════╗
║   💧 AGUADA Monitor Simples v1.0     ║
╠══════════════════════════════════════╣
║  Serial: {SERIAL_PORT:<26} ║
║  HTTP:   http://localhost:{HTTP_PORT:<10} ║
╚══════════════════════════════════════╝
""")
    
    # Thread serial
    t = threading.Thread(target=serial_thread, daemon=True)
    t.start()
    
    # Abrir navegador
    time.sleep(1)
    webbrowser.open(f'http://localhost:{HTTP_PORT}')
    
    # Servidor HTTP
    print(f"🌐 Servidor HTTP em http://localhost:{HTTP_PORT}")
    server = HTTPServer(('0.0.0.0', HTTP_PORT), Handler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n👋 Encerrado")
