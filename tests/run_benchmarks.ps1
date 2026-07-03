#!/usr/bin/env pwsh
# run_benchmarks.ps1
# Automated benchmark runner for memsim
# Pipes workload files through each allocator strategy, captures output

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$WorkloadDir = Join-Path $PSScriptRoot "workloads"
$ResultsDir  = Join-Path $PSScriptRoot "results"
$Executable  = Join-Path $ProjectRoot "memsim.exe"

# ── Build ──────────────────────────────────────────────────────────────────────
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  Building memsim..." -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Push-Location $ProjectRoot
try {
    & g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp src/physical_memory.cpp src/buddy_allocator.cpp src/cache.cpp src/virtual_memory.cpp -o memsim.exe 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "BUILD FAILED" -ForegroundColor Red
        exit 1
    }
    Write-Host "Build successful.`n" -ForegroundColor Green
} finally {
    Pop-Location
}

# ── Setup results directory ────────────────────────────────────────────────────
$Timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$RunDir = Join-Path $ResultsDir $Timestamp
New-Item -ItemType Directory -Path $RunDir -Force | Out-Null

# ── Define test matrix ─────────────────────────────────────────────────────────
# Allocator workloads: run through first_fit, best_fit, worst_fit, buddy
$AllocatorWorkloads = @(
    "workload_sequential_uniform.txt",
    "workload_random_mixed.txt",
    "workload_buddy_vs_standard.txt"
)

$Strategies = @("first_fit", "best_fit", "worst_fit", "buddy")

# Cache/VM workload: run once (allocator irrelevant)
$CacheWorkload = "workload_locality_heavy.txt"

# ── Helper: inject allocator command into workload ─────────────────────────────
function New-ModifiedWorkload {
    param(
        [string]$WorkloadPath,
        [string]$Strategy
    )

    $lines = Get-Content $WorkloadPath
    $result = @()

    foreach ($line in $lines) {
        $result += $line
        # After "init memory" line, inject the allocator switch
        if ($line -match "^init memory") {
            if ($Strategy -eq "buddy") {
                $result += "set allocator buddy"
            } else {
                $result += "set allocator $Strategy"
            }
        }
    }

    return $result
}

# ── Run allocator workloads ────────────────────────────────────────────────────
$AllResults = @()

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Running Allocator Benchmarks" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

foreach ($workloadFile in $AllocatorWorkloads) {
    $workloadPath = Join-Path $WorkloadDir $workloadFile
    $workloadName = [System.IO.Path]::GetFileNameWithoutExtension($workloadFile)

    if (-not (Test-Path $workloadPath)) {
        Write-Host "SKIP: $workloadFile not found" -ForegroundColor Yellow
        continue
    }

    foreach ($strategy in $Strategies) {
        $outFile = Join-Path $RunDir "${workloadName}_${strategy}.txt"

        Write-Host "  Running: $workloadName + $strategy ... " -NoNewline

        # Create modified workload with strategy injected
        $modifiedLines = New-ModifiedWorkload -WorkloadPath $workloadPath -Strategy $strategy

        # Pipe into memsim
        $output = $modifiedLines | & $Executable 2>&1
        $outputText = $output -join "`n"

        # Save raw output
        $outputText | Out-File -FilePath $outFile -Encoding utf8

        # Extract key metrics from output
        $util = ""
        $extFrag = ""
        $intFrag = ""
        $freeBlocks = ""
        $allocReqs = ""
        $successAllocs = ""
        $failedAllocs = ""

        foreach ($l in $output) {
            if ($l -match "Memory utilization:\s*([\d.]+)%") { $util = $Matches[1] }
            if ($l -match "External fragmentation:\s*([\d.]+)%") { $extFrag = $Matches[1] }
            if ($l -match "Internal fragmentation:\s*([\d.]+)%") { $intFrag = $Matches[1] }
            if ($l -match "Free blocks:\s*(\d+)") { $freeBlocks = $Matches[1] }
            if ($l -match "Alloc requests:\s*(\d+)") { $allocReqs = $Matches[1] }
            if ($l -match "Successful allocs:\s*(\d+)") { $successAllocs = $Matches[1] }
            if ($l -match "Failed allocs:\s*(\d+)") { $failedAllocs = $Matches[1] }
        }

        $AllResults += [PSCustomObject]@{
            Workload      = $workloadName
            Strategy      = $strategy
            Utilization   = $util
            ExtFrag       = $extFrag
            IntFrag       = if ($intFrag) { $intFrag } else { "N/A" }
            FreeBlocks    = $freeBlocks
            AllocReqs     = $allocReqs
            Success       = $successAllocs
            Failed        = $failedAllocs
        }

        Write-Host "done" -ForegroundColor Green
    }
    Write-Host ""
}

# ── Run cache/VM locality workload ─────────────────────────────────────────────
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Running Cache/VM Locality Benchmark" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

$cachePath = Join-Path $WorkloadDir $CacheWorkload
if (Test-Path $cachePath) {
    $outFile = Join-Path $RunDir "workload_locality_heavy.txt"

    Write-Host "  Running: locality_heavy ... " -NoNewline

    $output = Get-Content $cachePath | & $Executable 2>&1
    $outputText = $output -join "`n"
    $outputText | Out-File -FilePath $outFile -Encoding utf8

    Write-Host "done" -ForegroundColor Green

    # Extract cache/VM stats
    $cacheResults = @()
    $currentCache = ""
    $hits = ""
    $misses = ""
    foreach ($l in $output) {
        if ($l -match "(L[12]) Cache Stats") { $currentCache = $Matches[1] }
        if ($l -match "Hits:\s*(\d+)") {
            $hits = $Matches[1]
        }
        if ($l -match "Misses:\s*(\d+)") {
            $misses = $Matches[1]
        }
        if ($l -match "Hit Rate:\s*([\d.]+)%") {
            $cacheResults += [PSCustomObject]@{
                Cache   = $currentCache
                Hits    = $hits
                Misses  = $misses
                HitRate = $Matches[1]
            }
        }
        if ($l -match "Page faults:\s*(\d+)") {
            $pageFaults = $Matches[1]
        }
        if ($l -match "Page hits:\s*(\d+)") {
            $pageHits = $Matches[1]
        }
        if ($l -match "Page evictions:\s*(\d+)") {
            $pageEvictions = $Matches[1]
        }
    }
} else {
    Write-Host "SKIP: $CacheWorkload not found" -ForegroundColor Yellow
}

# ── Print Summary ──────────────────────────────────────────────────────────────
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  RESULTS SUMMARY" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "--- Allocator Comparison ---`n" -ForegroundColor Yellow
$AllResults | Format-Table -AutoSize

if ($cacheResults) {
    Write-Host "`n--- Cache Stats (cumulative at each checkpoint) ---`n" -ForegroundColor Yellow
    $cacheResults | Format-Table -AutoSize

    Write-Host "`n--- VM Stats (final) ---`n" -ForegroundColor Yellow
    Write-Host "  Page hits:      $pageHits"
    Write-Host "  Page faults:    $pageFaults"
    Write-Host "  Page evictions: $pageEvictions"
}

# ── Generate summary.md ───────────────────────────────────────────────────────
$summaryPath = Join-Path $RunDir "summary.md"
$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine("# Benchmark Results - $Timestamp")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("## Allocator Comparison")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("| Workload | Strategy | Utilization% | ExtFrag% | IntFrag% | FreeBlocks | Allocs | Success | Failed |")
[void]$sb.AppendLine("|----------|----------|-------------|----------|----------|------------|--------|---------|--------|")

foreach ($r in $AllResults) {
    [void]$sb.AppendLine("| $($r.Workload) | $($r.Strategy) | $($r.Utilization) | $($r.ExtFrag) | $($r.IntFrag) | $($r.FreeBlocks) | $($r.AllocReqs) | $($r.Success) | $($r.Failed) |")
}

if ($cacheResults) {
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("## Cache/VM Locality Results")
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("| Cache | Hits | Misses | Hit Rate% |")
    [void]$sb.AppendLine("|-------|------|--------|-----------|")
    foreach ($c in $cacheResults) {
        [void]$sb.AppendLine("| $($c.Cache) | $($c.Hits) | $($c.Misses) | $($c.HitRate) |")
    }
    [void]$sb.AppendLine("")
    [void]$sb.AppendLine("### Virtual Memory (final)")
    [void]$sb.AppendLine("- Page hits: $pageHits")
    [void]$sb.AppendLine("- Page faults: $pageFaults")
    [void]$sb.AppendLine("- Page evictions: $pageEvictions")
}

$sb.ToString() | Out-File -FilePath $summaryPath -Encoding utf8

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  Results saved to: $RunDir" -ForegroundColor Cyan
Write-Host "  Summary: $summaryPath" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan
