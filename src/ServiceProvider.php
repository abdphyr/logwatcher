<?php

namespace Abdphyr\Logwatcher;

use Illuminate\Support\ServiceProvider as LaravelServiceProvider;
use Abdphyr\Logwatcher\Console\Commands\StartLogwatcher;

class ServiceProvider extends LaravelServiceProvider
{
    public function boot()
    {
        $this->loadViewsFrom(__DIR__.'/../resources/views', 'logwatcher');

        $this->loadRoutesFrom(__DIR__ . '/../routes/web.php');

        if ($this->app->runningInConsole()) {
            $this->commands([StartLogwatcher::class]);
        }
    }

    public function register() {}
}
