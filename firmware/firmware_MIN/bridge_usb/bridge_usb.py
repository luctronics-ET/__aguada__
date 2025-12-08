#!/usr/bin/env python3
"""
AGUADA Bridge USB - Serial para HTTP/Backend

Lê JSON do gateway USB via Serial e envia para o backend via HTTP POST.

Uso:
    python bridge_usb.py [--port /dev/ttyACM0] [--backend http://localhost:3000]

Autor: AGUADA Project
Data: 2025-12-06
"""

import argparse
import json
import logging
import sys
import time
from datetime import datetime

import serial
import requests

# Configuração de logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)


class AguadaBridge:
    """Bridge USB para AGUADA - Serial para HTTP"""
    
    def __init__(self, port: str, baudrate: int, backend_url: str):
        self.port = port
        self.baudrate = baudrate
        self.backend_url = backend_url.rstrip('/')
        self.serial = None
        self.running = False
        
        # Estatísticas
        self.packets_received = 0
        self.packets_sent = 0
        self.packets_failed = 0
        self.last_packet_time = None
    
    def connect(self) -> bool:
        """Conecta à porta serial"""
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1
            )
            logger.info(f"Conectado em {self.port} @ {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            logger.error(f"Erro ao conectar: {e}")
            return False
    
    def send_to_backend(self, data: dict) -> bool:
        """Envia dados para o backend via HTTP POST"""
        url = f"{self.backend_url}/api/telemetry"
        
        try:
            response = requests.post(
                url,
                json=data,
                timeout=5,
                headers={'Content-Type': 'application/json'}
            )
            
            if response.status_code == 200 or response.status_code == 201:
                self.packets_sent += 1
                return True
            else:
                logger.warning(f"Backend retornou {response.status_code}: {response.text}")
                self.packets_failed += 1
                return False
                
        except requests.RequestException as e:
            logger.error(f"Erro HTTP: {e}")
            self.packets_failed += 1
            return False
    
    def process_line(self, line: str):
        """Processa uma linha JSON recebida"""
        line = line.strip()
        if not line or not line.startswith('{'):
            return
        
        try:
            data = json.loads(line)
            self.packets_received += 1
            self.last_packet_time = datetime.now()
            
            # Log do pacote
            mac = data.get('mac', 'unknown')
            pkt_type = data.get('type', 'unknown')
            value = data.get('value', '-')
            rssi = data.get('rssi', '-')
            
            logger.info(f"RX: {mac} | {pkt_type}={value} | RSSI={rssi}")
            
            # Ignora pacotes de status do gateway
            if pkt_type in ['gateway_status', 'gateway_boot']:
                logger.debug(f"Gateway status: {data}")
                return
            
            # Envia para backend
            if self.send_to_backend(data):
                logger.debug(f"TX: {mac} → backend OK")
            else:
                logger.warning(f"TX: {mac} → backend FAIL")
                
        except json.JSONDecodeError as e:
            logger.warning(f"JSON inválido: {line[:50]}... ({e})")
    
    def run(self):
        """Loop principal"""
        if not self.connect():
            return
        
        self.running = True
        logger.info(f"Bridge iniciada. Backend: {self.backend_url}")
        logger.info("Aguardando pacotes do gateway USB...")
        
        try:
            while self.running:
                try:
                    if self.serial.in_waiting > 0:
                        line = self.serial.readline().decode('utf-8', errors='ignore')
                        self.process_line(line)
                    else:
                        time.sleep(0.01)  # Evita busy-wait
                        
                except serial.SerialException as e:
                    logger.error(f"Erro serial: {e}")
                    logger.info("Tentando reconectar em 5s...")
                    time.sleep(5)
                    self.connect()
                    
        except KeyboardInterrupt:
            logger.info("\nEncerrando...")
        finally:
            self.running = False
            if self.serial:
                self.serial.close()
            
            # Estatísticas finais
            logger.info(f"Estatísticas: RX={self.packets_received}, "
                       f"TX={self.packets_sent}, FAIL={self.packets_failed}")
    
    def stop(self):
        """Para o bridge"""
        self.running = False


def main():
    parser = argparse.ArgumentParser(
        description='AGUADA Bridge USB - Serial para HTTP'
    )
    parser.add_argument(
        '--port', '-p',
        default='/dev/ttyACM0',
        help='Porta serial (default: /dev/ttyACM0)'
    )
    parser.add_argument(
        '--baudrate', '-b',
        type=int,
        default=115200,
        help='Baudrate (default: 115200)'
    )
    parser.add_argument(
        '--backend', '-u',
        default='http://localhost:3000',
        help='URL do backend (default: http://localhost:3000)'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Modo verbose (debug)'
    )
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    bridge = AguadaBridge(
        port=args.port,
        baudrate=args.baudrate,
        backend_url=args.backend
    )
    
    bridge.run()


if __name__ == '__main__':
    main()
