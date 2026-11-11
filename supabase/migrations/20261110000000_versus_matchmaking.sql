-- Supabase-backed matchmaking queue. Globed remains the gameplay/session transport.
create table if not exists public.versus_queue (
 ticket uuid primary key default gen_random_uuid(),
 mode text not null check (mode in ('race','survival','dual','platformer')),
 state text not null check (state in ('searching','ready-check','ready','cancelled')),
 created_at timestamptz not null default now(),
 ready boolean not null default false,
 opponent uuid references public.versus_queue(ticket) deferrable initially deferred,
 -- The Globed room the host created for this match, plus its passcode.
 globed_room_id text,
 globed_passcode text,
 globed_level_id integer,
 match_host boolean not null default false
);
create index if not exists versus_queue_search_idx on public.versus_queue(mode,state,created_at);
alter table public.versus_queue enable row level security;
revoke all on public.versus_queue from anon, authenticated;
grant all on public.versus_queue to service_role;
create or replace function public.versus_queue_join(p_mode text, p_room_id text, p_passcode text, p_level_id integer, p_is_host boolean) returns jsonb language plpgsql security definer set search_path=public,pg_temp as $$
declare v_ticket uuid; v_opponent public.versus_queue%rowtype; v_now timestamptz:=now();
begin
 if p_mode not in ('race','survival','dual','platformer') then raise exception 'invalid_mode'; end if;
 perform pg_advisory_xact_lock(hashtext('versus-global-queue'));
 delete from public.versus_queue where state='searching' and created_at<v_now-interval '2 minutes';
 select * into v_opponent from public.versus_queue where mode=p_mode and state='searching' and created_at>=v_now-interval '2 minutes' order by created_at for update skip locked limit 1;
 if found then
  v_ticket:=gen_random_uuid();
  insert into public.versus_queue(ticket,mode,state,opponent,match_host,globed_room_id,globed_passcode,globed_level_id) values(v_ticket,p_mode,'ready-check',v_opponent.ticket,true,coalesce(p_room_id,v_opponent.globed_room_id),coalesce(p_passcode,v_opponent.globed_passcode),coalesce(p_level_id,v_opponent.globed_level_id));
  update public.versus_queue set state='ready-check',opponent=v_ticket,globed_room_id=coalesce(globed_room_id,p_room_id),globed_passcode=coalesce(globed_passcode,p_passcode),globed_level_id=coalesce(globed_level_id,p_level_id) where ticket=v_opponent.ticket;
  return jsonb_build_object('ticket',v_ticket,'state','ready-check','mode',p_mode,'globedRoomId',coalesce(p_room_id,v_opponent.globed_room_id),'globedPasscode',coalesce(p_passcode,v_opponent.globed_passcode),'globedLevelId',coalesce(p_level_id,v_opponent.globed_level_id));
 end if;
 insert into public.versus_queue(mode,state,globed_room_id,globed_passcode,globed_level_id) values(p_mode,'searching',p_room_id,p_passcode,p_level_id) returning ticket into v_ticket;
 return jsonb_build_object('ticket',v_ticket,'state','searching','mode',p_mode);
end $$;
create or replace function public.versus_queue_status(p_ticket uuid) returns jsonb language sql security definer set search_path=public,pg_temp as $$
 select jsonb_build_object('state',q.state,'mode',q.mode,'createdAt',extract(epoch from q.created_at)*1000,'globedRoomId',q.globed_room_id,'globedPasscode',q.globed_passcode,'globedLevelId',q.globed_level_id) from public.versus_queue q where q.ticket=p_ticket;
$$;
create or replace function public.versus_match_ready(p_ticket uuid) returns jsonb language plpgsql security definer set search_path=public,pg_temp as $$
declare v_record public.versus_queue%rowtype; v_other public.versus_queue%rowtype;
begin
 perform pg_advisory_xact_lock(hashtext('versus-global-queue'));
 update public.versus_queue set state='cancelled',opponent=null where state in ('searching','ready-check') and created_at<now()-interval '2 minutes';
 select * into v_record from public.versus_queue where ticket=p_ticket for update;
 if not found then return jsonb_build_object('state','not_found'); end if;
 if v_record.state='ready' then return jsonb_build_object('state','ready','globedRoomId',v_record.globed_room_id,'globedPasscode',v_record.globed_passcode,'globedLevelId',v_record.globed_level_id); end if;
 if v_record.state<>'ready-check' then return jsonb_build_object('state',v_record.state); end if;
 update public.versus_queue set ready=true where ticket=p_ticket;
 if v_record.opponent is not null then select * into v_other from public.versus_queue where ticket=v_record.opponent for update; end if;
 if found and v_other.state='ready-check' and v_other.ready then
  update public.versus_queue set state='ready' where ticket in (p_ticket,v_record.opponent);
  return jsonb_build_object('state','ready','globedRoomId',v_record.globed_room_id,'globedPasscode',v_record.globed_passcode,'globedLevelId',v_record.globed_level_id);
 end if;
 return jsonb_build_object('state','ready-check');
end $$;
create or replace function public.versus_queue_leave(p_ticket uuid) returns jsonb language plpgsql security definer set search_path=public,pg_temp as $$
declare v_record public.versus_queue%rowtype;
begin
 perform pg_advisory_xact_lock(hashtext('versus-global-queue'));
 select * into v_record from public.versus_queue where ticket=p_ticket for update;
 if not found then return jsonb_build_object('state','not_found'); end if;
 if v_record.opponent is not null then update public.versus_queue set state='cancelled',opponent=null where ticket=v_record.opponent and state<>'ready'; end if;
 update public.versus_queue set state='cancelled' where ticket=p_ticket;
 return jsonb_build_object('state','cancelled');
end $$;
revoke all on function public.versus_queue_join(text,text,text,integer,boolean) from public,anon,authenticated;
revoke all on function public.versus_queue_status(uuid) from public,anon,authenticated;
revoke all on function public.versus_match_ready(uuid) from public,anon,authenticated;
revoke all on function public.versus_queue_leave(uuid) from public,anon,authenticated;
grant execute on function public.versus_queue_join(text,text,text,integer,boolean) to service_role;
grant execute on function public.versus_queue_status(uuid) to service_role;
grant execute on function public.versus_match_ready(uuid) to service_role;
grant execute on function public.versus_queue_leave(uuid) to service_role;
