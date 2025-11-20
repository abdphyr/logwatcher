<?php

namespace Abdphyr\Logwatcher;

use Illuminate\Support\ServiceProvider as LaravelServiceProvider;
use Abdphyr\Logwatcher\Console\Commands\StartLogwatcher;

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
