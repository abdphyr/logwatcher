<?php

namespace Logwatcher;

use Illuminate\Support\ServiceProvider as LaravelServiceProvider;
use Logwatcher\Console\Commands\StartLogwatcher;

class ServiceProvider extends LaravelServiceProvider
{
    public function boot()
    {
        if ($this->app->runningInConsole()) {
            $this->commands([StartLogwatcher::class]);
        }
    }

    public function register() {}
}
