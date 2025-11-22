#!/bin/bash

# ============================================================================
# AGUADA - Test MCP Servers
# ============================================================================

echo "🔍 Testing AGUADA MCP Servers..."
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ============================================================================
# 1. Test AGUADA MCP Server
# ============================================================================
echo "1️⃣  Testing AGUADA MCP Server..."

if [ -f "mcp-server/dist/index.js" ]; then
    echo -e "${GREEN}✅ AGUADA MCP compiled${NC}"
    
    # Test execution (timeout 2s)
    timeout 2 node mcp-server/dist/index.js < /dev/null > /tmp/mcp-test.log 2>&1 || true
    
    if grep -q "AGUADA MCP Server running" /tmp/mcp-test.log; then
        echo -e "${GREEN}✅ AGUADA MCP Server starts correctly${NC}"
    else
        echo -e "${RED}❌ AGUADA MCP Server failed to start${NC}"
        cat /tmp/mcp-test.log
    fi
else
    echo -e "${RED}❌ AGUADA MCP not compiled. Run: cd mcp-server && npm run build${NC}"
fi

echo ""

# ============================================================================
# 2. Test PostgreSQL Connection (for postgres MCP)
# ============================================================================
echo "2️⃣  Testing PostgreSQL Connection..."

if docker ps | grep -q aguada-postgres; then
    echo -e "${GREEN}✅ PostgreSQL Docker container running${NC}"
    
    # Test connection
    if docker exec aguada-postgres psql -U aguada_user -d aguada_db -c "SELECT 1;" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ PostgreSQL connection OK${NC}"
        
        # Count tables
        TABLE_COUNT=$(docker exec aguada-postgres psql -U aguada_user -d aguada_db -t -c "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='public';" 2>/dev/null | tr -d ' ')
        echo -e "${GREEN}   📊 Tables in database: ${TABLE_COUNT}${NC}"
    else
        echo -e "${RED}❌ PostgreSQL connection failed${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  PostgreSQL Docker not running. Start with: docker compose up -d postgres${NC}"
fi

echo ""

# ============================================================================
# 3. Check Filesystem Access
# ============================================================================
echo "3️⃣  Testing Filesystem Access..."

if [ -d "config" ] && [ -d "database" ] && [ -d "docs" ]; then
    echo -e "${GREEN}✅ Required directories exist:${NC}"
    echo -e "   - config/ ($(ls -1 config/*.json 2>/dev/null | wc -l) JSON files)"
    echo -e "   - database/ ($(ls -1 database/*.sql 2>/dev/null | wc -l) SQL files)"
    echo -e "   - docs/ ($(ls -1 docs/*.md 2>/dev/null | wc -l) MD files)"
else
    echo -e "${RED}❌ Missing required directories${NC}"
fi

echo ""

# ============================================================================
# 4. Check VS Code Settings
# ============================================================================
echo "4️⃣  Checking VS Code MCP Configuration..."

if [ -f ".vscode/mcp.json" ]; then
    echo -e "${GREEN}✅ mcp.json found in .vscode/${NC}"
    
    # Count configured servers
    SERVER_COUNT=$(grep -c '"command":\|"type":' .vscode/mcp.json || echo "0")
    echo -e "${GREEN}   🔌 MCP servers configured: $((SERVER_COUNT))${NC}"
    
    # Check for Figma servers
    if grep -q '"figma"' .vscode/mcp.json; then
        echo -e "${GREEN}   🎨 Figma MCP servers configured${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  .vscode/mcp.json not found${NC}"
fi

if [ -f ".vscode/settings.json" ]; then
    if grep -q "github.copilot.chat.mcp.enabled" .vscode/settings.json; then
        echo -e "${GREEN}✅ MCP enabled in VS Code settings${NC}"
    else
        echo -e "${YELLOW}⚠️  MCP not enabled in VS Code settings${NC}"
    fi
fi

echo ""

# ============================================================================
# 5. Check Figma MCP Connectivity
# ============================================================================
echo "5️⃣  Testing Figma MCP Servers..."

# Test remote Figma MCP
if curl -s --connect-timeout 3 https://mcp.figma.com/mcp > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Figma Remote MCP reachable${NC}"
else
    echo -e "${YELLOW}⚠️  Figma Remote MCP not reachable (check internet)${NC}"
fi

# Test local Figma Desktop MCP
if curl -s --connect-timeout 2 http://127.0.0.1:3845/mcp > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Figma Desktop MCP running (port 3845)${NC}"
else
    echo -e "${YELLOW}⚠️  Figma Desktop MCP not running (need Figma Desktop App)${NC}"
fi

echo ""

# ============================================================================
# 6. Summary
# ============================================================================
echo "📋 Summary:"
echo ""
echo "To activate MCP servers in VS Code:"
echo "1. Press Ctrl+Shift+P (Cmd+Shift+P on Mac)"
echo "2. Type: Developer: Reload Window"
echo "3. Press Enter"
echo ""
echo "Then use in Copilot Chat:"
echo "  @workspace Use aguada MCP to get reservoir status"
echo ""
echo "For more info, see: .vscode/MCP_SETUP.md"
echo ""

# Cleanup
rm -f /tmp/mcp-test.log
