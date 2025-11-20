<?php

namespace Logwatcher;

final class Server
{
    private static ?\FFI $ffi = null;

    private static function ffi(): \FFI
    {
        if (self::$ffi === null) {
            $header = <<<CDEF
                int start_server(const char *host, int port);
                int stop_server(void);
            CDEF;
            self::$ffi = \FFI::cdef($header, __DIR__ . '/native/server.so');
        }
        return self::$ffi;
    }

    public static function start(string $host, int $port): void
    {
        self::ffi()->{'start_server'}($host, $port);
        echo "Server started on $port \n";

        pcntl_signal(SIGINT, function () {
            echo "\nWatcher server stopped\n";
            self::ffi()->{'stop_server'}();
            exit;
        });

        while (true) {
            pcntl_signal_dispatch();
            sleep(1);
        }
    }
}
