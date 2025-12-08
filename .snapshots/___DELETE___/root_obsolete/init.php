#!/usr/bin/env php
<?php
/**
 * AGUADA - Script de Inicialização do Sistema
 * Configura e inicializa todos os componentes do sistema
 */

// Cores para terminal
class Colors {
    const GREEN = "\033[0;32m";
    const YELLOW = "\033[1;33m";
    const RED = "\033[0;31m";
    const BLUE = "\033[0;34m";
    const CYAN = "\033[0;36m";
    const NC = "\033[0m"; // No Color
    
    public static function success($text) {
        return self::GREEN . "[✓] " . self::NC . $text;
    }
    
    public static function warning($text) {
        return self::YELLOW . "[!] " . self::NC . $text;
    }
    
    public static function error($text) {
        return self::RED . "[✗] " . self::NC . $text;
    }
    
    public static function info($text) {
        return self::CYAN . "[i] " . self::NC . $text;
    }
}

// Configurações
$PROJECT_ROOT = "/home/luciano/Área de trabalho/aguada";
$BACKEND_DIR = "$PROJECT_ROOT/backend";
$FRONTEND_DIR = "$PROJECT_ROOT/frontend";
$DATABASE_DIR = "$PROJECT_ROOT/database";

// Configurações do banco
$DB_CONFIG = [
    'host' => 'localhost',
    'port' => 5432,
    'user' => 'aguada_user',
    'password' => 'aguada_pass_2025',
    'database' => 'aguada_db',
    'schema' => 'aguada'
];

// Banner
function printBanner() {
    echo "\n";
    echo "╔════════════════════════════════════════════════════════════╗\n";
    echo "║  AGUADA - Inicialização do Sistema                        ║\n";
    echo "║  Monitoramento Hidráulico de Reservatórios                ║\n";
    echo "╚════════════════════════════════════════════════════════════╝\n";
    echo "\n";
}

// Verificar comando disponível
function commandExists($command) {
    $output = [];
    $return = 0;
    exec("which $command 2>/dev/null", $output, $return);
    return $return === 0;
}

// Executar comando e retornar resultado
function execCommand($command, &$output = null, &$returnCode = null) {
    exec($command . " 2>&1", $output, $returnCode);
    return $returnCode === 0;
}

// Verificar pré-requisitos
function checkPrerequisites() {
    echo Colors::info("1. Verificando pré-requisitos...\n\n");
    
    $allOk = true;
    
    // Node.js
    if (commandExists('node')) {
        $version = trim(shell_exec('node --version'));
        echo Colors::success("Node.js encontrado: $version\n");
    } else {
        echo Colors::error("Node.js não instalado\n");
        $allOk = false;
    }
    
    // npm
    if (commandExists('npm')) {
        $version = trim(shell_exec('npm --version'));
        echo Colors::success("npm encontrado: $version\n");
    } else {
        echo Colors::error("npm não instalado\n");
        $allOk = false;
    }
    
    // PostgreSQL
    if (commandExists('psql')) {
        $version = trim(shell_exec('psql --version'));
        echo Colors::success("PostgreSQL encontrado: $version\n");
    } else {
        echo Colors::error("PostgreSQL não instalado\n");
        $allOk = false;
    }
    
    // Verificar se PostgreSQL está rodando
    $pgRunning = execCommand("pg_isready -h localhost 2>/dev/null");
    if ($pgRunning) {
        echo Colors::success("PostgreSQL está rodando\n");
    } else {
        echo Colors::warning("PostgreSQL não está rodando. Execute: sudo systemctl start postgresql\n");
    }
    
    echo "\n";
    return $allOk;
}

// Configurar banco de dados
function setupDatabase($config) {
    echo Colors::info("2. Configurando banco de dados...\n\n");
    
    $commands = [];
    
    // Criar usuário (se não existir)
    $createUser = "sudo -u postgres psql -c \"SELECT 1 FROM pg_user WHERE usename='{$config['user']}'\" -t 2>/dev/null";
    $userExists = trim(shell_exec($createUser));
    
    if (empty($userExists)) {
        echo Colors::info("Criando usuário do banco...\n");
        $cmd = "sudo -u postgres psql -c \"CREATE USER {$config['user']} WITH PASSWORD '{$config['password']}';\" 2>&1";
        exec($cmd, $output, $return);
        if ($return === 0) {
            echo Colors::success("Usuário criado\n");
        } else {
            echo Colors::warning("Usuário pode já existir\n");
        }
    } else {
        echo Colors::success("Usuário já existe\n");
    }
    
    // Criar banco (se não existir)
    $dbExists = execCommand("sudo -u postgres psql -lqt | cut -d \\| -f 1 | grep -qw {$config['database']}");
    if (!$dbExists) {
        echo Colors::info("Criando banco de dados...\n");
        $cmd = "sudo -u postgres psql -c \"CREATE DATABASE {$config['database']} OWNER {$config['user']};\" 2>&1";
        exec($cmd, $output, $return);
        if ($return === 0) {
            echo Colors::success("Banco de dados criado\n");
        } else {
            echo Colors::error("Erro ao criar banco: " . implode("\n", $output) . "\n");
            return false;
        }
    } else {
        echo Colors::success("Banco de dados já existe\n");
    }
    
    // Criar schema
    echo Colors::info("Criando schema...\n");
    $schemaFile = __DIR__ . "/database/schema.sql";
    if (file_exists($schemaFile)) {
        $cmd = "sudo -u postgres psql -d {$config['database']} -f $schemaFile 2>&1";
        exec($cmd, $output, $return);
        if ($return === 0) {
            echo Colors::success("Schema criado/atualizado\n");
        } else {
            echo Colors::warning("Schema pode já existir ou erro: " . implode("\n", array_slice($output, -3)) . "\n");
        }
    } else {
        // Criar schema básico via SQL direto
        $sql = "
        CREATE SCHEMA IF NOT EXISTS {$config['schema']};
        
        CREATE TABLE IF NOT EXISTS {$config['schema']}.sensores (
            sensor_id VARCHAR(50) PRIMARY KEY,
            elemento_id VARCHAR(50) NOT NULL,
            node_mac VARCHAR(17),
            variavel VARCHAR(50) NOT NULL,
            tipo VARCHAR(50),
            status VARCHAR(20) DEFAULT 'ativo'
        );
        
        CREATE TABLE IF NOT EXISTS {$config['schema']}.leituras_raw (
            leitura_id SERIAL PRIMARY KEY,
            sensor_id VARCHAR(50) NOT NULL,
            elemento_id VARCHAR(50) NOT NULL,
            variavel VARCHAR(50) NOT NULL,
            valor NUMERIC(10,2) NOT NULL,
            unidade VARCHAR(10) DEFAULT 'cm',
            datetime TIMESTAMP DEFAULT NOW(),
            fonte VARCHAR(50),
            autor VARCHAR(50),
            modo VARCHAR(20),
            meta JSONB,
            FOREIGN KEY (sensor_id) REFERENCES {$config['schema']}.sensores(sensor_id)
        );
        
        CREATE INDEX IF NOT EXISTS idx_leituras_datetime ON {$config['schema']}.leituras_raw(datetime DESC);
        CREATE INDEX IF NOT EXISTS idx_leituras_sensor ON {$config['schema']}.leituras_raw(sensor_id, datetime DESC);
        
        GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA {$config['schema']} TO {$config['user']};
        GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA {$config['schema']} TO {$config['user']};
        ";
        
        $tempFile = tempnam(sys_get_temp_dir(), 'aguada_schema');
        file_put_contents($tempFile, $sql);
        $cmd = "sudo -u postgres psql -d {$config['database']} -f $tempFile 2>&1";
        exec($cmd, $output, $return);
        unlink($tempFile);
        
        if ($return === 0) {
            echo Colors::success("Schema e tabelas criados\n");
        } else {
            echo Colors::warning("Tabelas podem já existir\n");
        }
    }
    
    echo "\n";
    return true;
}

// Configurar backend
function setupBackend($backendDir, $dbConfig) {
    echo Colors::info("3. Configurando backend...\n\n");
    
    if (!is_dir($backendDir)) {
        echo Colors::error("Diretório backend não encontrado: $backendDir\n");
        return false;
    }
    
    // Criar .env se não existir
    $envFile = "$backendDir/.env";
    if (!file_exists($envFile)) {
        echo Colors::info("Criando arquivo .env...\n");
        $envContent = "PORT=3000
DB_HOST={$dbConfig['host']}
DB_PORT={$dbConfig['port']}
DB_USER={$dbConfig['user']}
DB_PASSWORD={$dbConfig['password']}
DB_NAME={$dbConfig['database']}
SERIAL_PORT=/dev/ttyACM0
SERIAL_BAUD=115200
";
        file_put_contents($envFile, $envContent);
        echo Colors::success("Arquivo .env criado\n");
    } else {
        echo Colors::success("Arquivo .env já existe\n");
    }
    
    // Instalar dependências
    echo Colors::info("Verificando dependências do Node.js...\n");
    if (!is_dir("$backendDir/node_modules")) {
        echo Colors::info("Instalando dependências (isso pode levar alguns minutos)...\n");
        chdir($backendDir);
        exec("npm install 2>&1", $output, $return);
        if ($return === 0) {
            echo Colors::success("Dependências instaladas\n");
        } else {
            echo Colors::error("Erro ao instalar dependências\n");
            return false;
        }
    } else {
        echo Colors::success("Dependências já instaladas\n");
    }
    
    echo "\n";
    return true;
}

// Verificar frontend
function checkFrontend($frontendDir) {
    echo Colors::info("4. Verificando frontend...\n\n");
    
    if (file_exists("$frontendDir/index.html")) {
        echo Colors::success("Dashboard HTML encontrado\n");
        return true;
    } else {
        echo Colors::warning("Dashboard HTML não encontrado em $frontendDir/index.html\n");
        return false;
    }
}

// Verificar serviços
function checkServices() {
    echo Colors::info("5. Verificando serviços...\n\n");
    
    // Backend API
    $backendRunning = @file_get_contents("http://localhost:3000/api/health");
    if ($backendRunning !== false) {
        echo Colors::success("Backend API está rodando (porta 3000)\n");
    } else {
        echo Colors::warning("Backend API não está rodando\n");
        echo Colors::info("  Para iniciar: cd backend && npm start\n");
    }
    
    // PostgreSQL
    $pgRunning = execCommand("pg_isready -h localhost 2>/dev/null");
    if ($pgRunning) {
        echo Colors::success("PostgreSQL está rodando\n");
    } else {
        echo Colors::warning("PostgreSQL não está rodando\n");
    }
    
    echo "\n";
}

// Exibir informações finais
function printSummary($projectRoot, $backendDir, $frontendDir) {
    echo Colors::info("6. Informações do Sistema:\n\n");
    
    echo "  📡 Backend API:       <http://localhost:3000/api>\n";
    echo "  🏥 Health Check:      <http://localhost:3000/api/health>\n";
    echo "  📊 Dashboard:         <http://localhost:8080>\n";
    echo "  🗄️  Database:          PostgreSQL @ localhost:5432\n";
    echo "\n";
    
    echo Colors::info("7. Próximos passos:\n\n");
    echo "  Para INICIAR O BACKEND:\n";
    echo "    cd $backendDir\n";
    echo "    npm start\n";
    echo "\n";
    echo "  Para ACESSAR O DASHBOARD:\n";
    echo "    Abra no navegador: <http://localhost:8080>\n";
    echo "\n";
    echo "  Para TESTAR A API:\n";
    echo "    curl <http://localhost:3000/api/health>\n";
    echo "\n";
}

// Função principal
function main() {
    global $PROJECT_ROOT, $BACKEND_DIR, $FRONTEND_DIR, $DATABASE_DIR, $DB_CONFIG;
    
    printBanner();
    
    // Mudar para diretório do projeto
    if (!is_dir($PROJECT_ROOT)) {
        echo Colors::error("Diretório do projeto não encontrado: $PROJECT_ROOT\n");
        exit(1);
    }
    chdir($PROJECT_ROOT);
    
    // Verificar pré-requisitos
    if (!checkPrerequisites()) {
        echo Colors::error("Pré-requisitos não atendidos. Instale Node.js, npm e PostgreSQL.\n");
        exit(1);
    }
    
    // Configurar banco de dados
    if (!setupDatabase($DB_CONFIG)) {
        echo Colors::error("Erro ao configurar banco de dados\n");
        exit(1);
    }
    
    // Configurar backend
    if (!setupBackend($BACKEND_DIR, $DB_CONFIG)) {
        echo Colors::error("Erro ao configurar backend\n");
        exit(1);
    }
    
    // Verificar frontend
    checkFrontend($FRONTEND_DIR);
    
    // Verificar serviços
    checkServices();
    
    // Resumo final
    printSummary($PROJECT_ROOT, $BACKEND_DIR, $FRONTEND_DIR);
    
    echo Colors::success("Sistema inicializado com sucesso!\n\n");
}

// Executar
main();

