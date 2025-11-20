<?php

namespace Abdphyr\Logwatcher\Http\Controllers;

use Illuminate\Http\Request;

class LogwatcherController
{
    public function ui()
    {
        
        return view('logwatcher::ui');
    }

    public function folder(Request $request)
    {
        $folder = $request?->folder ?? storage_path('logs');
        if (!is_dir($folder)) return [];
        $result = [];
        $scanned = scandir($folder);
        foreach ($scanned as $item) {
            if ($item == '.' || $item == '..') {
                continue;
            }
            $result[] = [
                'is_dir' => is_dir($path = "$folder/$item"),
                'name' => $item,
                'path' => $path
            ];
        }
        return $result;
    }
}