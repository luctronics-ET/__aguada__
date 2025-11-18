#!/bin/bash

# =============================================================================
# AGUADA - DEPLOY CHECKLIST
# Execute este script para validar instalação completa
# =============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASSED=0
FAILED=0
WARNINGS=0

log_pass() { echo -e "${GREEN}✅${NC} $1"; ((PASSED++)); }
log_fail() { echo -e "${RED}❌${NC} $1"; ((FAILED++)); }
log_warn() { echo -e "${YELLOW}⚠️ ${NC} $1"; ((WARNINGS++)); }
log_info() { echo -e "${BLUE}ℹ️ ${NC}$1"; }

# =============================================================================
# HEADER
# =============================================================================

clear
echo -e "${BLUE}"
echo "=========================================="
echo "  AGUADA - Deploy Validation Checklist"
echo "=========================================="
echo -e "${NC}\n"

# =============================================================================
# 1. DOCKER CHECKS
# =============================================================================

echo -e "${BLUE}1. Docker Installation${NC}"

if command -v docker &> /dev/null; then
    DOCKER_VERSION=$(docker --version)
    log_pass "Docker installed: $DOCKER_VERSION"
else
    log_fail "Docker not installed"
fi

if command -v docker-compose &> /dev/null; then
    DOCKER_COMPOSE_VERSION=$(docker-compose --version)
    log_pass "Docker Compose installed: $DOCKER_COMPOSE_VERSION"
else
    log_fail "Docker Compose not installed"
fi

echo ""

# =============================================================================
# 2. CONTAINERS
# =============================================================================

echo -e "${BLUE}2. Docker Containers${NC}"

# PostgreSQL
if docker ps 2>/dev/null | grep -q aguada-postgres; then
    log_pass "PostgreSQL container is running"
else
    log_warn "PostgreSQL container not running (docker-compose up -d)"
fi

# Redis
if docker ps 2>/dev/null | grep -q aguada-redis; then
    log_pass "Redis container is running"
else
    log_warn "Redis container not running"
fi

# Backend
if docker ps 2>/dev/null | grep -q aguada-backend; then
    log_pass "Backend container is running"
else
    log_warn "Backend container not running"
fi

# Nginx
if docker ps 2>/dev/null | grep -q aguada-nginx; then
    log_pass "Nginx container is running"
else
    log_warn "Nginx container not running"
fi

echo ""

# =============================================================================
# 3. DATABASE
# =============================================================================

echo -e "${BLUE}3. Database Connectivity${NC}"

if docker exec aguada-postgres pg_isready -U aguada_user -d aguada_db &> /dev/null 2>&1; then
    log_pass "PostgreSQL is responding"
    
    # Check tables
    TABLES=$(docker exec aguada-postgres psql -U aguada_user -d aguada_db -t -c "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='aguada';" 2>/dev/null || echo "0")
    if [ "$TABLES" -gt 0 ]; then
        log_pass "Database schema exists ($TABLES tables)"
    else
        log_warn "Database schema not initialized (run: psql -f database/init.sql)"
    fi
    
    # Check sensors
    SENSORS=$(docker exec aguada-postgres psql -U aguada_user -d aguada_db -t -c "SELECT COUNT(*) FROM sensores;" 2>/dev/null || echo "0")
    if [ "$SENSORS" -eq 5 ]; then
        log_pass "5 sensors configured"
    elif [ "$SENSORS" -gt 0 ]; then
        log_warn "Found $SENSORS sensors (expected 5)"
    else
        log_warn "No sensors configured"
    fi
else
    log_fail "PostgreSQL not responding"
fi

echo ""

# =============================================================================
# 4. REDIS
# =============================================================================

echo -e "${BLUE}4. Redis Cache${NC}"

if docker exec aguada-redis redis-cli ping 2>/dev/null | grep -q "PONG"; then
    log_pass "Redis is responding"
else
    log_fail "Redis not responding"
fi

echo ""

# =============================================================================
# 5. BACKEND API
# =============================================================================

echo -e "${BLUE}5. Backend API (Node.js)${NC}"

# Health endpoint
if curl -s http://localhost:3000/api/health 2>/dev/null | grep -q "ok"; then
    log_pass "Backend health endpoint responding"
    
    # Check version
    VERSION=$(curl -s http://localhost:3000/api/health 2>/dev/null | grep -o '"version":"[^"]*"' | cut -d'"' -f4)
    log_info "Backend version: $VERSION"
else
    log_fail "Backend not responding on port 3000"
fi

# Readings endpoint
if curl -s http://localhost:3000/api/readings/latest 2>/dev/null | grep -q "sensor_id"; then
    READING_COUNT=$(curl -s http://localhost:3000/api/readings/latest 2>/dev/null | grep -o '"sensor_id"' | wc -l)
    log_pass "Readings endpoint working ($READING_COUNT readings)"
else
    log_warn "Readings endpoint not responding"
fi

echo ""

# =============================================================================
# 6. FRONTEND
# =============================================================================

echo -e "${BLUE}6. Frontend Web Server${NC}"

if curl -s http://localhost/ 2>/dev/null | grep -q "<!DOCTYPE"; then
    log_pass "Frontend web server responding"
    
    # Check pages
    PAGES=("index.html" "history.html" "alerts.html" "system.html" "config.html")
    for page in "${PAGES[@]}"; do
        if curl -s http://localhost/"$page" 2>/dev/null | grep -q "DOCTYPE"; then
            log_pass "  - $page found"
        else
            log_warn "  - $page not found"
        fi
    done
else
    log_fail "Frontend not responding on port 80"
fi

echo ""

# =============================================================================
# 7. TELEMETRY DATA
# =============================================================================

echo -e "${BLUE}7. Telemetry Data${NC}"

# Count readings
READING_COUNT=$(docker exec aguada-postgres psql -U aguada_user -d aguada_db -t -c "SELECT COUNT(*) FROM leituras_raw;" 2>/dev/null || echo "0")
if [ "$READING_COUNT" -gt 0 ]; then
    log_pass "Telemetry data received ($READING_COUNT readings)"
    
    # Check recent data (last 5 minutes)
    RECENT=$(docker exec aguada-postgres psql -U aguada_user -d aguada_db -t -c "SELECT COUNT(*) FROM leituras_raw WHERE tempo > NOW() - INTERVAL '5 minutes';" 2>/dev/null || echo "0")
    if [ "$RECENT" -gt 0 ]; then
        log_pass "Recent data received ($RECENT readings in last 5 min)"
    else
        log_warn "No recent data (sensors may be offline)"
    fi
else
    log_warn "No telemetry data yet (sensors may not be connected)"
fi

echo ""

# =============================================================================
# 8. FILE STRUCTURE
# =============================================================================

echo -e "${BLUE}8. File Structure${NC}"

FILES=(
    "docker-compose.yml"
    "backend/Dockerfile"
    "docker/nginx.conf"
    "database/init.sql"
    "scripts/deploy.sh"
    "scripts/install-manual.sh"
    "scripts/migration.sh"
    "frontend/index.html"
    "frontend/assets/style.css"
    "frontend/assets/app.js"
    "DEPLOY_GUIDE.md"
    "QUICKSTART.md"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        log_pass "$file"
    else
        log_fail "$file (not found)"
    fi
done

echo ""

# =============================================================================
# 9. DISK SPACE
# =============================================================================

echo -e "${BLUE}9. System Resources${NC}"

# Disk space
DISK_FREE=$(df -h . | tail -1 | awk '{print $4}')
DISK_USAGE=$(du -sh . | awk '{print $1}')
log_info "Project size: $DISK_USAGE"
log_info "Free disk space: $DISK_FREE"

# Docker disk usage
if command -v docker &> /dev/null; then
    DOCKER_VOLUME=$(docker ps --no-trunc -aqf "name=aguada" -q 2>/dev/null | xargs docker inspect --format='{{.State.Pid}}' 2>/dev/null | head -1 || echo "unknown")
    log_info "Docker volumes available"
fi

echo ""

# =============================================================================
# 10. NETWORK
# =============================================================================

echo -e "${BLUE}10. Network Connectivity${NC}"

# Check ports
PORTS=(80 3000 5432 6379)
for port in "${PORTS[@]}"; do
    if nc -z localhost "$port" 2>/dev/null; then
        log_pass "Port $port is open"
    else
        log_warn "Port $port is not accessible"
    fi
done

# Gateway IP
IP=$(hostname -I | awk '{print $1}')
log_info "Local IP: $IP"

echo ""

# =============================================================================
# 11. PERFORMANCE
# =============================================================================

echo -e "${BLUE}11. Performance Metrics${NC}"

# Response time
START=$(date +%s%N)
curl -s http://localhost:3000/api/health > /dev/null 2>&1
END=$(date +%s%N)
RESPONSE_TIME=$(( (END - START) / 1000000 ))
if [ "$RESPONSE_TIME" -lt 500 ]; then
    log_pass "API response time: ${RESPONSE_TIME}ms (fast)"
elif [ "$RESPONSE_TIME" -lt 1000 ]; then
    log_warn "API response time: ${RESPONSE_TIME}ms (acceptable)"
else
    log_fail "API response time: ${RESPONSE_TIME}ms (slow)"
fi

# Container memory
if docker stats --no-stream --format "table {{.MemUsage}}" aguada-backend 2>/dev/null; then
    log_info "Docker containers running normally"
fi

echo ""

# =============================================================================
# SUMMARY
# =============================================================================

echo -e "${BLUE}=========================================="
echo "  SUMMARY"
echo "==========================================${NC}\n"

echo -e "${GREEN}✅ Passed: $PASSED${NC}"
echo -e "${RED}❌ Failed: $FAILED${NC}"
echo -e "${YELLOW}⚠️  Warnings: $WARNINGS${NC}\n"

# =============================================================================
# RECOMMENDATIONS
# =============================================================================

echo -e "${BLUE}📋 RECOMMENDATIONS${NC}\n"

if [ $FAILED -gt 0 ]; then
    echo "❌ Fix the following issues:"
    echo "   - docker-compose up -d      (if containers not running)"
    echo "   - psql -f database/init.sql (if database not initialized)"
    echo ""
fi

if [ $WARNINGS -gt 0 ]; then
    echo "⚠️  Consider:"
    echo "   - Check ESP32 sensor connections"
    echo "   - Verify gateway configuration"
    echo "   - Run: docker-compose logs to see detailed errors"
    echo ""
fi

if [ $FAILED -eq 0 ] && [ $WARNINGS -le 2 ]; then
    echo -e "${GREEN}✨ System is running properly!${NC}\n"
    echo "Next steps:"
    echo "  1. Check frontend: http://localhost"
    echo "  2. Monitor backend: docker logs -f aguada-backend"
    echo "  3. Wait for sensors to connect"
    echo "  4. View dashboard updates in real-time"
    echo ""
fi

# =============================================================================
# USEFUL COMMANDS
# =============================================================================

echo -e "${BLUE}🔧 USEFUL COMMANDS${NC}\n"
echo "  View logs:        docker-compose logs -f"
echo "  Access database:  docker exec -it aguada-postgres psql -U aguada_user -d aguada_db"
echo "  Restart all:      docker-compose restart"
echo "  Reset everything: docker-compose down -v"
echo "  Test API:         curl http://localhost:3000/api/health"
echo ""

echo -e "${BLUE}=========================================="
echo "  Validation Complete ✅"
echo "==========================================${NC}\n"
