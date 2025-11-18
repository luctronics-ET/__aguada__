/* AGUADA - WebSocket Client */

class WebSocketClient {
    constructor(url) {
        this.url = url || 'ws://192.168.0.100:3000/ws';
        this.ws = null;
        this.reconnectInterval = 5000; // 5 seconds
        this.reconnectTimer = null;
        this.listeners = {};
        this.isConnecting = false;
        this.maxReconnectAttempts = 10;
        this.reconnectAttempts = 0;
    }

    /**
     * Connect to WebSocket server
     */
    connect() {
        if (this.isConnecting || (this.ws && this.ws.readyState === WebSocket.OPEN)) {
            return;
        }

        this.isConnecting = true;
        console.log('[WebSocket] Connecting to:', this.url);

        try {
            this.ws = new WebSocket(this.url);

            this.ws.onopen = () => {
                console.log('[WebSocket] Connected');
                this.isConnecting = false;
                this.reconnectAttempts = 0;
                this.emit('connected');
                
                // Clear reconnect timer
                if (this.reconnectTimer) {
                    clearTimeout(this.reconnectTimer);
                    this.reconnectTimer = null;
                }
            };

            this.ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    console.log('[WebSocket] Message received:', data);
                    
                    // Emit specific event type
                    if (data.type) {
                        this.emit(data.type, data);
                    }
                    
                    // Emit general message event
                    this.emit('message', data);
                } catch (error) {
                    console.error('[WebSocket] Error parsing message:', error);
                }
            };

            this.ws.onerror = (error) => {
                console.error('[WebSocket] Error:', error);
                this.isConnecting = false;
                this.emit('error', error);
            };

            this.ws.onclose = (event) => {
                console.log('[WebSocket] Disconnected:', event.code, event.reason);
                this.isConnecting = false;
                this.emit('disconnected', event);
                
                // Attempt to reconnect
                if (this.reconnectAttempts < this.maxReconnectAttempts) {
                    this.scheduleReconnect();
                } else {
                    console.error('[WebSocket] Max reconnect attempts reached');
                    this.emit('maxReconnectAttemptsReached');
                }
            };

        } catch (error) {
            console.error('[WebSocket] Connection error:', error);
            this.isConnecting = false;
            this.scheduleReconnect();
        }
    }

    /**
     * Schedule reconnection attempt
     */
    scheduleReconnect() {
        if (this.reconnectTimer) return;
        
        this.reconnectAttempts++;
        const delay = this.reconnectInterval * Math.min(this.reconnectAttempts, 5);
        
        console.log(`[WebSocket] Reconnecting in ${delay/1000}s (attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
        
        this.reconnectTimer = setTimeout(() => {
            this.reconnectTimer = null;
            this.connect();
        }, delay);
    }

    /**
     * Send message to server
     */
    send(data) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(data));
            console.log('[WebSocket] Message sent:', data);
            return true;
        } else {
            console.warn('[WebSocket] Cannot send - not connected');
            return false;
        }
    }

    /**
     * Subscribe to event
     */
    on(event, callback) {
        if (!this.listeners[event]) {
            this.listeners[event] = [];
        }
        this.listeners[event].push(callback);
    }

    /**
     * Unsubscribe from event
     */
    off(event, callback) {
        if (!this.listeners[event]) return;
        
        this.listeners[event] = this.listeners[event].filter(cb => cb !== callback);
    }

    /**
     * Emit event to listeners
     */
    emit(event, data) {
        if (!this.listeners[event]) return;
        
        this.listeners[event].forEach(callback => {
            try {
                callback(data);
            } catch (error) {
                console.error(`[WebSocket] Error in ${event} listener:`, error);
            }
        });
    }

    /**
     * Disconnect from server
     */
    disconnect() {
        if (this.reconnectTimer) {
            clearTimeout(this.reconnectTimer);
            this.reconnectTimer = null;
        }
        
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
        
        this.reconnectAttempts = this.maxReconnectAttempts; // Prevent auto-reconnect
        console.log('[WebSocket] Disconnected by user');
    }

    /**
     * Get connection state
     */
    getState() {
        if (!this.ws) return 'CLOSED';
        
        switch (this.ws.readyState) {
            case WebSocket.CONNECTING: return 'CONNECTING';
            case WebSocket.OPEN: return 'OPEN';
            case WebSocket.CLOSING: return 'CLOSING';
            case WebSocket.CLOSED: return 'CLOSED';
            default: return 'UNKNOWN';
        }
    }

    /**
     * Check if connected
     */
    isConnected() {
        return this.ws && this.ws.readyState === WebSocket.OPEN;
    }
}

// Create global WebSocket instance
const wsClient = new WebSocketClient();

// Auto-connect on load (can be disabled if needed)
if (typeof window !== 'undefined') {
    // Wait a bit before connecting to ensure page is loaded
    setTimeout(() => {
        wsClient.connect();
    }, 1000);

    // Reconnect on page visibility change
    document.addEventListener('visibilitychange', () => {
        if (!document.hidden && !wsClient.isConnected()) {
            wsClient.connect();
        }
    });
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { WebSocketClient, wsClient };
}
