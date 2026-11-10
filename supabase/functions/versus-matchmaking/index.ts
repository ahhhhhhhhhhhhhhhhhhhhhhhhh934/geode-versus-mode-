import { createClient } from "https://esm.sh/@supabase/supabase-js@2";
const cors={ "access-control-allow-origin":"*", "access-control-allow-methods":"GET, POST, OPTIONS", "access-control-allow-headers":"Content-Type, Authorization, apikey", "content-type":"application/json; charset=utf-8", "cache-control":"no-store" };
function reply(body:unknown,status=200){return new Response(JSON.stringify(body),{status,headers:cors});}
function ticketFrom(request:Request):string|null{const h=request.headers.get("Authorization")??"";const t=h.startsWith("Bearer ")?h.slice(7).trim():"";return /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i.test(t)?t:null;}
Deno.serve(async(request:Request)=>{
 if(request.method==="OPTIONS")return new Response(null,{headers:cors});
 const path=new URL(request.url).pathname.split("/").filter(Boolean).slice(-2).join("/");
 const url=Deno.env.get("SUPABASE_URL"),key=Deno.env.get("SUPABASE_SERVICE_ROLE_KEY");
 if(!url||!key)return reply({error:"server_not_configured"},503);
 const db=createClient(url,key,{auth:{persistSession:false,autoRefreshToken:false}});
 if(path==="queue/join"&&request.method==="POST"){
  let body:{mode?:string};try{body=await request.json();}catch{return reply({error:"invalid_json"},400);}
  const mode=String(body.mode??"").toLowerCase();if(!["race","survival","dual","platformer"].includes(mode))return reply({error:"invalid_mode"},400);
  const {data,error}=await db.rpc("versus_queue_join",{p_mode:mode});if(error)return reply({error:"queue_failed"},500);return reply(data);
 }
 const ticket=ticketFrom(request);if(!ticket)return reply({error:"missing_or_invalid_ticket"},401);
 if(path==="queue/status"&&request.method==="GET"){const {data,error}=await db.rpc("versus_queue_status",{p_ticket:ticket});if(error)return reply({error:"status_failed"},500);return data?reply(data):reply({state:"not_found"},404);}
 if(path==="match/ready"&&request.method==="POST"){const {data,error}=await db.rpc("versus_match_ready",{p_ticket:ticket});if(error)return reply({error:"ready_failed"},500);return reply(data);}
 if(path==="queue/leave"&&request.method==="POST"){const {data,error}=await db.rpc("versus_queue_leave",{p_ticket:ticket});if(error)return reply({error:"leave_failed"},500);return reply(data);}
 return reply({error:"not_found"},404);
});
