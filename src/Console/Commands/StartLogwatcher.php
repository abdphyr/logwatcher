<?php

namespace Logwatcher\Console\Commands;

use Illuminate\Console\Command;
use Logwatcher\Server;

class StartLogwatcher extends Command
{
    protected $signature = 'start-logwatcher {--host= : Host} {--port= : Port}';

    protected $description = 'Start log watcher';

    public function handle()
    {
        $parts = parse_url(config('app.url'));
        $host = $this->option('host') ?? $parts['host'] ?? '127.0.0.1';
        $port = $this->option('port') ?? $parts['port'] ?? 8000;
        return Server::start($host, $port);
    }
}
