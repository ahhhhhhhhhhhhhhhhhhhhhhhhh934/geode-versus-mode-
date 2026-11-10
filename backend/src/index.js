const JSON_HEADERS = {
  "content-type": "application/json; charset=utf-8",
  "access-control-allow-origin": "*",
  "access-control-allow-methods": "GET, POST, OPTIONS",
  "access-control-allow-headers": "Content-Type, Authorization",
  "cache-control": "no-store",
};

function json(data, status = 200) {
  return new Response(JSON.stringify(data), { status, headers: JSON_HEADERS });
}

function bearer(request) {
  const value = request.headers.get("Authorization") || "";
  return value.startsWith("Bearer ") ? value.slice(7).trim() : "";
}

export default {
  async fetch(request, env) {
    if (request.method === "OPTIONS") return new Response(null, { headers: JSON_HEADERS });
    const url = new URL(request.url);
    const match = url.pathname.match(/^\/(queue\/join|queue\/status|queue\/leave|match\/ready)$/);
    if (!match) return json({ error: "not_found" }, 404);

    let mode = "race";
    if (request.method === "POST" && match[1] === "queue/join") {
      let body;
      try { body = await request.json(); } catch { return json({ error: "invalid_json" }, 400); }
      mode = String(body.mode || "").toLowerCase();
      if (!["race", "survival", "dual", "platformer"].includes(mode)) {
        return json({ error: "invalid_mode" }, 400);
      }
      const id = env.MATCHMAKER.idFromName(mode);
      return env.MATCHMAKER.get(id).fetch("https://matchmaker/queue/join", {
        method: "POST", headers: { "content-type": "application/json" },
        body: JSON.stringify({ mode }),
      });
    }

    const ticket = bearer(request);
    if (!ticket) return json({ error: "missing_ticket" }, 401);
    const id = env.MATCHMAKER.idFromName("global");
    return env.MATCHMAKER.get(id).fetch("https://matchmaker/" + match[1], {
      method: request.method,
      headers: { "authorization": "Bearer " + ticket },
    });
  },
};

export class Matchmaker {
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
  }

  async fetch(request) {
    const url = new URL(request.url);
    const route = url.pathname;
    if (route === "/queue/join" && request.method === "POST") return this.join(request);
    const ticket = (request.headers.get("Authorization") || "").replace(/^Bearer\s+/, "");
    if (!ticket) return json({ error: "missing_ticket" }, 401);
    const key = "ticket:" + ticket;
    const record = await this.ctx.storage.get(key);
    if (!record) return json({ state: "not_found" }, 404);

    if (route === "/queue/status" && request.method === "GET") {
      return json(this.publicState(record));
    }
    if (route === "/match/ready" && request.method === "POST") return this.ready(key, record);
    if (route === "/queue/leave" && request.method === "POST") return this.leave(key, record);
    return json({ error: "not_found" }, 404);
  }

  publicState(record) {
    return {
      state: record.state,
      mode: record.mode,
      sessionId: record.sessionId || null,
      createdAt: record.createdAt,
    };
  }

  async join(request) {
    const body = await request.json();
    const mode = body.mode;
    const now = Date.now();
    const ticket = crypto.randomUUID();
    const waiting = await this.ctx.storage.list({ prefix: "ticket:" });
    let opponentKey = null;
    let opponent = null;

    for (const [key, value] of waiting) {
      if (value.state === "searching" && now - value.createdAt < 120000) {
        opponentKey = key;
        opponent = value;
        break;
      }
      if (value.state === "searching") {
        await this.ctx.storage.delete(key);
      }
    }

    const own = {
      state: opponent ? "ready-check" : "searching",
      mode,
      createdAt: now,
      ready: false,
      sessionId: opponent ? crypto.randomUUID() : null,
      opponent: opponentKey ? opponentKey.slice("ticket:".length) : null,
    };

    if (opponent && opponentKey) {
      const sessionId = crypto.randomUUID();
      own.sessionId = sessionId;
      opponent.state = "ready-check";
      opponent.sessionId = sessionId;
      opponent.opponent = ticket;
      await this.ctx.storage.put(opponentKey, opponent);
    }

    await this.ctx.storage.put("ticket:" + ticket, own);
    return json({ ticket, state: own.state, mode });
  }

  async ready(key, record) {
    if (record.state === "ready") return json({ state: "ready", sessionId: record.sessionId });
    if (record.state !== "ready-check") return json({ state: record.state });
    record.ready = true;
    await this.ctx.storage.put(key, record);
    const otherKey = record.opponent ? "ticket:" + record.opponent : null;
    const other = otherKey ? await this.ctx.storage.get(otherKey) : null;
    if (other && other.state === "ready-check" && other.ready) {
      record.state = "ready";
      other.state = "ready";
      await this.ctx.storage.put(key, record);
      await this.ctx.storage.put(otherKey, other);
      return json({ state: "ready", sessionId: record.sessionId });
    }
    return json({ state: "ready-check", sessionId: record.sessionId });
  }

  async leave(key, record) {
    const otherKey = record.opponent ? "ticket:" + record.opponent : null;
    const other = otherKey ? await this.ctx.storage.get(otherKey) : null;
    if (other && other.state !== "ready") {
      other.state = "cancelled";
      other.opponent = null;
      await this.ctx.storage.put(otherKey, other);
    }
    record.state = "cancelled";
    await this.ctx.storage.put(key, record);
    return json({ state: "cancelled" });
  }
}
