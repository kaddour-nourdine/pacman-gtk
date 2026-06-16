package com.pacman

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.*
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.material3.Text
import kotlinx.coroutines.delay
import kotlin.math.abs
import kotlin.math.sin
import kotlin.math.sqrt

const val MAP_W = 21
const val MAP_H = 21

enum class AppMode { MENU, PLAYING, REPLAY }

class MainActivity : ComponentActivity() {
    private val model = PacmanGame()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        model.init()
        setContent { PacmanApp(model) }
    }
}

@Composable
fun PacmanApp(model: PacmanGame) {
    var appMode by remember { mutableStateOf(AppMode.MENU) }
    var menuSel by remember { mutableIntStateOf(0) }

    when (appMode) {
        AppMode.MENU -> MenuScreen(
            model = model,
            menuSel = menuSel,
            onPlay = {
                model.reset()
                model.startRecording()
                appMode = AppMode.PLAYING
            },
            onReplay = {
                if (model.hasRecording()) {
                    model.startPlayback()
                    appMode = AppMode.REPLAY
                }
            }
        )
        AppMode.PLAYING, AppMode.REPLAY -> GameScreen(
            model = model,
            appMode = appMode,
            onBackToMenu = { appMode = AppMode.MENU }
        )
    }
}

@Composable
fun MenuScreen(
    model: PacmanGame,
    menuSel: Int,
    onPlay: () -> Unit,
    onReplay: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black)
            .pointerInput(Unit) {
                detectTapGestures { offset ->
                    val h = size.height.toFloat()
                    val upper = h / 3
                    val lower = 2 * h / 3
                    if (offset.y < upper) {
                        /* UP — no-op, just visual selection */
                    } else if (offset.y < lower) {
                        /* confirm selection */
                    }
                }
            },
        contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                text = "PACMAN",
                color = Color.Yellow,
                fontSize = 52.sp,
                fontWeight = FontWeight.Bold
            )

            Spacer(Modifier.height(8.dp))

            Text(
                text = "by Nourdine Kaddour",
                color = Color.Gray,
                fontSize = 14.sp
            )

            Spacer(Modifier.height(80.dp))

            val items = listOf("Play Game", "Watch Demo")
            items.forEachIndexed { i, label ->
                val sel = menuSel == i
                Text(
                    text = if (sel) "\u203A $label \u2039" else "  $label  ",
                    color = if (sel) Color.Yellow else Color(0xFF666666),
                    fontSize = if (sel) 30.sp else 22.sp,
                    fontWeight = if (sel) FontWeight.Bold else FontWeight.Normal,
                    modifier = Modifier
                        .padding(vertical = 4.dp)
                        .pointerInput(i) {
                            detectTapGestures {
                                if (i == 0) onPlay()
                                else if (model.hasRecording()) onReplay()
                            }
                        }
                )
                if (i == 1 && !model.hasRecording()) {
                    Text(
                        text = "(no recording - play a game first)",
                        color = Color(0xFF444444),
                        fontSize = 12.sp
                    )
                }
            }
        }
    }
}

@Composable
fun GameScreen(
    model: PacmanGame,
    appMode: AppMode,
    onBackToMenu: () -> Unit
) {
    val d = LocalConfiguration.current
    val density = d.densityDpi / 160f
    val sw = d.screenWidthDp.dp.value * density
    val sh = d.screenHeightDp.dp.value * density
    val ts = (sw / 23f).coerceAtMost(sh / 26f)
    val ox = (sw - MAP_W * ts) / 2f
    val oy = (sh - MAP_H * ts) / 2f - 30f * density

    var gameOverTimer by remember { mutableFloatStateOf(0f) }
    var prevGameOver by remember { mutableStateOf(false) }

    LaunchedEffect(appMode) {
        var lastMs = System.nanoTime() / 1_000_000L
        while (true) {
            val now = System.nanoTime() / 1_000_000L
            val dt = (now - lastMs) / 1000.0
            lastMs = now

            if (appMode == AppMode.REPLAY && model.isGameOver()) {
                if (!prevGameOver) { gameOverTimer = 4f; prevGameOver = true }
                gameOverTimer -= dt.toFloat()
                if (gameOverTimer <= 0f) {
                    model.reset()
                    prevGameOver = false
                    onBackToMenu()
                    return@LaunchedEffect
                }
            } else {
                model.update(dt)
                prevGameOver = false
            }
            delay(16)
        }
    }

    Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
        Canvas(
            modifier = Modifier
                .fillMaxSize()
                .pointerInput(appMode) {
                    detectTapGestures { pos ->
                        if (appMode != AppMode.PLAYING) return@detectTapGestures
                        val rx = pos.x - ox
                        val ry = pos.y - oy
                        if (rx < 0 || rx >= MAP_W * ts || ry < 0 || ry >= MAP_H * ts) return@detectTapGestures
                        val tx = (rx / ts).toInt()
                        val ty = (ry / ts).toInt()
                        val pp = model.getPacmanPos()
                        val px = pp[0].toInt(); val py = pp[1].toInt()
                        val dx = tx - px; val dy = ty - py
                        val dir = when {
                            abs(dx) > abs(dy) && dx != 0 -> if (dx > 0) Direction.RIGHT else Direction.LEFT
                            dy != 0 -> if (dy > 0) Direction.DOWN else Direction.UP
                            else -> Direction.NONE
                        }
                        if (dir != Direction.NONE) model.setDirection(dir)
                    }
                }
        ) {
            val c = drawContext.canvas.nativeCanvas

            drawRect(Color.Black, Offset.Zero, size)

            for (y in 0 until MAP_H) {
                for (x in 0 until MAP_W) {
                    val tile = model.getTile(x, y)
                    val px = ox + x * ts
                    val py = oy + y * ts
                    when (tile) {
                        TileType.WALL -> drawRect(
                            Color.Blue, Offset(px + 2, py + 2),
                            Size(ts - 4, ts - 4), style = Stroke(2f)
                        )
                        TileType.PELLET -> drawCircle(
                            Color.White, ts * 0.1f, Offset(px + ts / 2, py + ts / 2)
                        )
                        TileType.POWER_PELLET -> drawCircle(
                            Color.White, ts * 0.22f, Offset(px + ts / 2, py + ts / 2)
                        )
                    }
                }
            }

            drawPacman(model, ox, oy, ts)
            drawGhosts(model, ox, oy, ts)

            val uiY = oy + MAP_H * ts + 10
            val label = if (appMode == AppMode.REPLAY)
                "REPLAY    Score: ${model.getScore()}"
            else
                "Score: ${model.getScore()}  Lives: ${model.getLives()}"
            c.drawText(label, ox, uiY, android.graphics.Paint().apply {
                color = android.graphics.Color.WHITE
                textSize = 24f
                isAntiAlias = true
            })

            if (model.isGameOver()) {
                val txt = if (appMode == AppMode.REPLAY) "REPLAY OVER" else "GAME OVER"
                c.drawText(txt, ox + MAP_W * ts / 2 - 90f, oy + MAP_H * ts / 2,
                    android.graphics.Paint().apply {
                        color = android.graphics.Color.RED
                        textSize = 48f
                        isAntiAlias = true
                    })
            }
        }
    }
}

fun DrawScope.drawPacman(model: PacmanGame, ox: Float, oy: Float, ts: Float) {
    val p = model.getPacmanPos()
    val px = p[0]; val py = p[1]; val dir = p[2].toInt()
    val cx = ox + (px * ts).toFloat() + ts / 2
    val cy = oy + (py * ts).toFloat() + ts / 2
    val r = ts / 2 - 2

    val mouth = abs(sin(System.currentTimeMillis() * 0.01)) * 30.0
    val color = if (model.isDying()) Color.Red else Color.Yellow

    val facing = when (dir) {
        Direction.RIGHT -> 0.0; Direction.DOWN -> 90.0
        Direction.LEFT -> 180.0; Direction.UP -> 270.0
        else -> 0.0
    }
    val startA = (facing + mouth).toFloat()
    val sweepA = (360f - 2f * mouth).toFloat()

    val path = Path().apply {
        moveTo(cx, cy)
        arcTo(
            Rect(cx - r, cy - r, cx + r, cy + r),
            startA, sweepA, false
        )
        close()
    }
    drawPath(path, color)
}

fun DrawScope.drawGhosts(model: PacmanGame, ox: Float, oy: Float, ts: Float) {
    val ghosts = model.getGhosts()
    val colors = listOf(Color.Red, Color(0xFFFFB3B3), Color.Cyan, Color(0xFFFF9900))

    ghosts.forEachIndexed { i, g ->
        val gx = g[0]; val gy = g[1]; val fri = g[3].toInt() != 0
        val cx = ox + (gx * ts).toFloat() + ts / 2
        val cy = oy + (gy * ts).toFloat() + ts / 2
        val r = ts / 2 - 2
        val col = if (fri) Color.Blue else colors[i % colors.size]

        drawArc(col, 180f, 180f, false, Offset(cx - r, cy - r), Size(2 * r, 2 * r))
        drawRect(col, Offset(cx - r, cy), Size(2 * r, r - 2))

        val eyeOff = ts * 0.33f; val eyeY = ts * 0.4f; val eyeR = ts * 0.13f
        val pupilR = ts * 0.07f
        drawCircle(Color.White, eyeR, Offset(cx - eyeOff, cy - r + eyeY))
        drawCircle(Color.White, eyeR, Offset(cx + eyeOff, cy - r + eyeY))
        drawCircle(Color.Black, pupilR, Offset(cx - eyeOff, cy - r + eyeY))
        drawCircle(Color.Black, pupilR, Offset(cx + eyeOff, cy - r + eyeY))
    }
}
