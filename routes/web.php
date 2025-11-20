<?php

use Abdphyr\Logwatcher\Http\Controllers\LogwatcherController;
use Illuminate\Support\Facades\Route;

Route::prefix('logwatcher')->group(function () {
    Route::get('ui', [LogwatcherController::class, 'ui'])->name('ui');
    Route::post('folder', [LogwatcherController::class, 'folder'])->name('folder');
});