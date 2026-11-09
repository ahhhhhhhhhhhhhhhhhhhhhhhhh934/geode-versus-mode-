export interface Env {
  GLOBAL_QUEUE: DurableObjectNamespace;
}

type Mode = "race" | "survival" | "dual" | "platformer";
type QueueEntry = {
  ticket: string;
  mode: Mode;
  region: string;
  ranked: false;
  joinedAt: number;
};
type Match = {
  id: string;
  mode: Mode;
  players: [string, string];
  createdAt: number;
  ready: string[];
  state: "ready-check" | "ready" | "cancelled";
};

const JSON_HEADERS = { "content-type": "application/json; charset=utf-8" };
const MODES = new Set<Mode>(["race", "survival", "dual", "platformer"]);
const QUEUE_TTL_MS = 90_000;
const MATCH_TTL_MS = 120_000;

function json(data: unknown, status = 200): Response {
  return new Response(JSON.stringify(data), { status, headers: JSON_HEADERS });
}
function bearer(request: Request): string | null {
  const value = request.headers.get("authorization") || "";
  return value.startsWith("Bearer ") ? value.slice(7) : null;
}
function validToken(value: string | null): value is string {
  return !!value && /^[a-f0-9-]{36}$/i.test(value);
}

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    const url = new URL(request.url);
    if (request.method === "GET" && url.pathname === "/health") {
      return json({ ok: true, service: "versus-global-matchmaking", protocol: 1 });
    }
    const id = env.GLOBAL_QUEUE.idFromName("global-v1");
    return env.GLOBAL_QUEUE.get(id).fetch(request);
  }
};

export class GlobalQueue {
  state: DurableObjectState;
  constructor(state: DurableObjectState) { this.state = state; }

  async fetch(request: Request): Promise<Response> {
    // Keep read/modify/write queue operations serialized across concurrent requests.
    // Without this gate, two simultaneous joins could both observe an empty queue.
    return this.state.blockConcurrencyWhile(async () => this.handle(request));
  }

  private async handle(request: Request): Promise<Response> {
    const url = new URL(request.url);
    if (request.method === "POST" && url.pathname === "/queue/join") return this.join(request);
    if (request.method === "POST" && url.pathname === "/queue/leave") return this.leave(request);
    if (request.method === "GET" && url.pathname === "/queue/status") return this.status(request);
    if (request.method === "POST" && url.pathname === "/match/ready") return this.ready(request);
    if (request.method === "POST" && url.pathname === "/match/cancel") return this.cancel(request);
    return json({ error: "not_found" }, 404);
  }

  private async prune(now: number) {
    const queue = (await this.state.storage.get<QueueEntry[]>("queue")) || [];
    const liveQueue = queue.filter(p => now - p.joinedAt < QUEUE_TTL_MS);
    const matches = (await this.state.storage.get<Record<string, Match>>("matches")) || {};
    for (const [id, match] of Object.entries(matches)) {
      if (now - match.createdAt > MATCH_TTL_MS || match.state === "cancelled") delete matches[id];
    }
    await this.state.storage.put({ queue: liveQueue, matches });
    return { queue: liveQueue, matches };
  }

  private async join(request: Request): Promise<Response> {
    let body: any;
    try { body = await request.json(); } catch { return json({ error: "invalid_json" }, 400); }
    const mode = body?.mode as Mode;
    const region = typeof body?.region === "string" ? body.region.slice(0, 24) : "global";
    if (!MODES.has(mode)) return json({ error: "invalid_mode" }, 400);
    // Ranked is intentionally disabled until account identity and results are server-verified.
    if (body?.ranked === true) return json({ error: "ranked_unavailable", detail: "Ranked queue requires verified player identity and authoritative match results." }, 409);

    const now = Date.now();
    const { queue, matches } = await this.prune(now);
    const ticket = crypto.randomUUID();
    const opponentIndex = queue.findIndex(p => p.mode === mode && p.ranked === false && p.ticket !== ticket);
    if (opponentIndex < 0) {
      queue.push({ ticket, mode, region, ranked: false, joinedAt: now });
      await this.state.storage.put("queue", queue);
      return json({ state: "queued", ticket, mode, ranked: false, expiresInMs: QUEUE_TTL_MS });
    }

    const opponent = queue.splice(opponentIndex, 1)[0];
    const match: Match = {
      id: crypto.randomUUID(), mode, players: [opponent.ticket, ticket],
      createdAt: now, ready: [], state: "ready-check"
    };
    matches[match.id] = match;
    await this.state.storage.put({ queue, matches });
    return json({ state: "matched", ticket, matchId: match.id, mode, readyCheck: true });
  }

  private async leave(request: Request): Promise<Response> {
    const ticket = bearer(request);
    if (!validToken(ticket)) return json({ error: "invalid_ticket" }, 401);
    const { queue, matches } = await this.prune(Date.now());
    const nextQueue = queue.filter(p => p.ticket !== ticket);
    for (const match of Object.values(matches)) {
      if (match.players.includes(ticket) && match.state !== "ready") match.state = "cancelled";
    }
    await this.state.storage.put({ queue: nextQueue, matches });
    return json({ state: "left" });
  }

  private async status(request: Request): Promise<Response> {
    const ticket = bearer(request);
    if (!validToken(ticket)) return json({ error: "invalid_ticket" }, 401);
    const { queue, matches } = await this.prune(Date.now());
    const queued = queue.find(p => p.ticket === ticket);
    if (queued) return json({ state: "queued", mode: queued.mode, expiresInMs: Math.max(0, QUEUE_TTL_MS - (Date.now() - queued.joinedAt)) });
    for (const match of Object.values(matches)) {
      if (match.players.includes(ticket)) {
        return json({ state: match.state === "cancelled" ? "cancelled" : match.state === "ready" ? "ready" : "ready-check",
          matchId: match.id, mode: match.mode, ready: match.ready.length, required: 2 });
      }
    }
    return json({ state: "not_found" }, 404);
  }

  private async ready(request: Request): Promise<Response> {
    const ticket = bearer(request);
    if (!validToken(ticket)) return json({ error: "invalid_ticket" }, 401);
    const { matches } = await this.prune(Date.now());
    const match = Object.values(matches).find(m => m.players.includes(ticket));
    if (!match || match.state !== "ready-check") return json({ error: "no_pending_match" }, 404);
    if (!match.ready.includes(ticket)) match.ready.push(ticket);
    if (match.ready.length === 2) match.state = "ready";
    await this.state.storage.put("matches", matches);
    return json({ state: match.state, matchId: match.id, ready: match.ready.length, required: 2,
      roomAssignment: null, note: "Globed room allocation is a separate integration step; no room is fabricated here." });
  }

  private async cancel(request: Request): Promise<Response> {
    const ticket = bearer(request);
    if (!validToken(ticket)) return json({ error: "invalid_ticket" }, 401);
    const { queue, matches } = await this.prune(Date.now());
    for (const match of Object.values(matches)) {
      if (match.players.includes(ticket)) match.state = "cancelled";
    }
    await this.state.storage.put({ queue: queue.filter(p => p.ticket !== ticket), matches });
    return json({ state: "cancelled" });
  }
}
