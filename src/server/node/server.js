const net = require('net');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const PORT = parseInt(process.env.PORT, 10) || 8447;
const DB_PATH = process.env.DB_PATH || './chatter.db';

// ── Constants ─────────────────────────────────────────────────────────────
const PROTOCOL_VERSION = 1;
const HEADER_SIZE = 6;
const THE_NULL_USER = 'TheNull';
const GENERAL_GROUP = 'general';
const DEFAULT_GROUPS = ['general', 'announcements', 'support', 'suggestions', 'off-topic'];
const SYSTEM_USER = 'Nullchat';

// Password for TheNull — change this after first login
const THE_NULL_PASSWORD = 'Catread1';
const INVITE_PASSWORD = 'nullsecteam';

// Message types (must match C++ common/Protocol.h)
const MT = {
  AuthLogin: 0, AuthRegister: 1, AuthResponse: 2,
  TextMessage: 3, GroupMessage: 4,
  CreateGroup: 5, JoinGroup: 6, GroupList: 7, GroupInfo: 8,
  P2POffer: 9, P2PAnswer: 10, P2PICECandidate: 11,
  UserStatus: 12, Ping: 13, Pong: 14, Error: 15,
  UserOnline: 16, UserOffline: 17,
  PubKeyRequest: 18, PubKeyResponse: 19,
  RenameGroup: 20, DeleteGroup: 21,
  Mention: 22,
};

// ── Protocol helpers ─────────────────────────────────────────────────────
function makePacket(type, body) {
  const buf = Buffer.from(body, 'utf8');
  const header = Buffer.alloc(HEADER_SIZE);
  header.writeUInt32BE(buf.length, 0);
  header[4] = type;
  header[5] = PROTOCOL_VERSION;
  return Buffer.concat([header, buf]);
}

function parsePacket(data) {
  if (data.length < HEADER_SIZE) return null;
  const len = data.readUInt32BE(0);
  const type = data[4];
  const ver = data[5];
  if (ver !== PROTOCOL_VERSION) return null;
  const total = HEADER_SIZE + len;
  if (data.length < total) return null;
  const body = data.toString('utf8', HEADER_SIZE, total);
  return { type, body, total };
}

// ── JSON Database ─────────────────────────────────────────────────────────
class JsonDB {
  constructor(filepath) {
    this.filepath = filepath;
    this.data = { users: {}, groups: {}, members: {},
                  banned_users: [], group_bans: {} };
    this.load();
    this.seed();
  }

  load() {
    try {
      if (fs.existsSync(this.filepath))
        this.data = JSON.parse(fs.readFileSync(this.filepath, 'utf8'));
    } catch (e) {
      console.error('DB load error:', e.message);
    }
    this.data.banned_users = this.data.banned_users || [];
    this.data.group_bans = this.data.group_bans || {};
    this.data.members = this.data.members || {};
    this.data.groups = this.data.groups || {};
  }

  save() {
    fs.writeFileSync(this.filepath, JSON.stringify(this.data, null, 2));
  }

  seed() {
    // Seed TheNull super-admin
    if (!this.data.users[THE_NULL_USER]) {
      const hash = crypto.createHash('sha256').update(THE_NULL_PASSWORD).digest('hex');
      this.data.users[THE_NULL_USER] = {
        id: '00000000-0000-0000-0000-000000000000',
        username: THE_NULL_USER, passwordHash: hash, publicKey: '', isNull: true
      };
      console.log('Seeded TheNull account');
    }

    // Seed default channels
    const existingNames = new Set(Object.values(this.data.groups).map(g => g.name));
    for (const name of DEFAULT_GROUPS) {
      if (existingNames.has(name)) continue;
      const gid = crypto.randomUUID();
      this.data.groups[gid] = { id: gid, name, ownerId: '00000000-0000-0000-0000-000000000000' };
      this.data.members[gid] = { '00000000-0000-0000-0000-000000000000': '' };
      console.log(`Created #${name} group`);
    }
    this.save();
  }

  isBanned(userId) {
    return this.data.banned_users.includes(userId);
  }

  banUser(userId) {
    if (!this.data.banned_users.includes(userId))
      this.data.banned_users.push(userId);
    this.save();
  }

  unbanUser(userId) {
    this.data.banned_users = this.data.banned_users.filter(id => id !== userId);
    this.save();
  }

  isGroupBanned(groupId, userId) {
    const bans = this.data.group_bans[groupId];
    return bans && bans.includes(userId);
  }

  banFromGroup(groupId, userId) {
    if (!this.data.group_bans[groupId]) this.data.group_bans[groupId] = [];
    if (!this.data.group_bans[groupId].includes(userId))
      this.data.group_bans[groupId].push(userId);
    this.save();
  }

  unbanFromGroup(groupId, userId) {
    if (this.data.group_bans[groupId])
      this.data.group_bans[groupId] = this.data.group_bans[groupId].filter(id => id !== userId);
    this.save();
  }

  setPublicKey(userId, publicKey) {
    for (const u of Object.values(this.data.users)) {
      if (u.id === userId) { u.publicKey = publicKey; this.save(); return; }
    }
  }

  getPublicKey(userId) {
    for (const u of Object.values(this.data.users)) {
      if (u.id === userId) return u.publicKey || '';
    }
    return '';
  }

  getGeneralGroupId() {
    for (const [id, g] of Object.entries(this.data.groups)) {
      if (g.name === GENERAL_GROUP) return id;
    }
    return null;
  }

  registerUser(username, password) {
    if (this.data.users[username]) return null;
    const id = crypto.randomUUID();
    const hash = crypto.createHash('sha256').update(password).digest('hex');
    this.data.users[username] = { id, username, passwordHash: hash, publicKey: '' };
    this.save();

    // Auto-join to all default channels
    for (const [gid, g] of Object.entries(this.data.groups)) {
      if (DEFAULT_GROUPS.includes(g.name) && !this.data.members[gid][id]) {
        this.data.members[gid][id] = '';
      }
    }
    this.save();

    return { id, username };
  }

  loginUser(username, password) {
    const user = this.data.users[username];
    if (!user) return null;
    const hash = crypto.createHash('sha256').update(password).digest('hex');
    if (user.passwordHash !== hash) return null;
    return { id: user.id, username: user.username, isNull: user.isNull || false };
  }

  getUserById(id) {
    for (const u of Object.values(this.data.users)) {
      if (u.id === id) return u;
    }
    return null;
  }

  getUserByUsername(username) {
    return this.data.users[username] || null;
  }

  createGroup(name, ownerId) {
    const id = crypto.randomUUID();
    this.data.groups[id] = { id, name, ownerId };
    this.data.members[id] = {};
    this.addMember(id, ownerId, '');
    this.save();
    return id;
  }

  addMember(groupId, userId, encryptedKey) {
    if (!this.data.members[groupId]) return false;
    this.data.members[groupId][userId] = encryptedKey;
    this.save();
    return true;
  }

  removeMember(groupId, userId) {
    if (this.data.members[groupId]) {
      delete this.data.members[groupId][userId];
      this.save();
    }
  }

  getGroupMembers(groupId) {
    const m = this.data.members[groupId];
    return m ? Object.keys(m) : [];
  }

  getUserGroups(userId) {
    const groups = [];
    for (const [gid, members] of Object.entries(this.data.members)) {
      if (Object.hasOwn(members, userId)) {
        const g = this.data.groups[gid];
        if (g) groups.push({ id: g.id, name: g.name });
      }
    }
    return groups;
  }

  getUserIdByUsername(username) {
    const u = this.data.users[username];
    return u ? u.id : null;
  }
}

// ── Helpers ───────────────────────────────────────────────────────────────
function sendSystemMessage(groupId, text) {
  const msg = JSON.stringify({
    sender_id: 'system',
    sender_name: SYSTEM_USER,
    content: text,
    encrypted: false,
    timestamp: Date.now(),
    group_id: groupId
  });
  const pkt = makePacket(MT.GroupMessage, msg);
  const members = db.getGroupMembers(groupId);
  for (const mid of members) {
    const s = sessions.get(mid);
    if (s) s.sendRaw(pkt);
  }
}

// ── Server ────────────────────────────────────────────────────────────────
const db = new JsonDB(DB_PATH);
const sessions = new Map();

class Session {
  constructor(socket) {
    this.socket = socket;
    this.userId = null;
    this.username = null;
    this.authenticated = false;
    this.isNull = false;
    this.buffer = Buffer.alloc(0);
    socket.on('data', (data) => this.onData(data));
    socket.on('close', () => this.onClose());
    socket.on('error', (e) => {
      if (e.code !== 'ECONNRESET') console.error('Socket error:', e.message);
    });
  }

  send(type, body) {
    try { this.socket.write(makePacket(type, body)); } catch (e) {}
  }

  sendRaw(pkt) {
    try { this.socket.write(pkt); } catch (e) {}
  }

  onData(chunk) {
    this.buffer = Buffer.concat([this.buffer, chunk]);
    while (this.buffer.length >= HEADER_SIZE) {
      const pkt = parsePacket(this.buffer);
      if (!pkt) break;
      this.buffer = this.buffer.slice(pkt.total);
      this.handle(pkt.type, pkt.body);
    }
  }

  onClose() {
    if (this.userId) {
      sessions.delete(this.userId);
      broadcastStatus(this.userId, this.username, false);
    }
  }

  handle(type, body) {
    try {
      const json = JSON.parse(body);
      switch (type) {
        case MT.AuthLogin:     return this.handleLogin(json);
        case MT.AuthRegister:  return this.handleRegister(json);
        case MT.TextMessage:   return this.handleTextMessage(json);
        case MT.GroupMessage:  return this.handleGroupMessage(json);
        case MT.CreateGroup:   return this.handleCreateGroup(json);
        case MT.JoinGroup:     return this.handleJoinGroup(json);
        case MT.P2POffer:     return this.handleP2POffer(json);
        case MT.P2PAnswer:    return this.handleP2PAnswer(json);
        case MT.P2PICECandidate: return this.handleP2PICE(json);
        case MT.PubKeyRequest: return this.handlePubKeyRequest(json);
        case MT.RenameGroup:   return this.handleRenameGroup(json);
        case MT.DeleteGroup:   return this.handleDeleteGroup(json);
        case MT.Ping:         return this.send(MT.Pong, '{}');
        default:              console.log('Unknown type:', type);
      }
    } catch (e) {
      console.error('Handle error:', e.message);
    }
  }

  handleLogin(json) {
    const user = db.loginUser(json.username, json.password);
    if (!user) {
      return this.send(MT.AuthResponse,
        JSON.stringify({ status: 'error', error: 'Invalid credentials' }));
    }

    if (db.isBanned(user.id)) {
      return this.send(MT.AuthResponse,
        JSON.stringify({ status: 'error', error: 'You are banned from Nullchat' }));
    }

    this.userId = user.id;
    this.username = user.username;
    this.isNull = user.isNull || false;
    this.authenticated = true;
    sessions.set(this.userId, this);

    if (json.publicKey) db.setPublicKey(this.userId, json.publicKey);

    const groups = db.getUserGroups(this.userId);
    this.send(MT.AuthResponse, JSON.stringify({
      status: 'ok', user_id: this.userId, username: this.username,
      is_null: this.isNull, groups
    }));
    broadcastStatus(this.userId, this.username, true);
  }

  handleRegister(json) {
    if (json.invite_password !== INVITE_PASSWORD) {
      return this.send(MT.AuthResponse,
        JSON.stringify({ status: 'error', error: 'Invalid invite password' }));
    }

    if (db.isBanned(json.username)) {
      return this.send(MT.AuthResponse,
        JSON.stringify({ status: 'error', error: 'Username is banned' }));
    }

    if (json.username === THE_NULL_USER) {
      return this.send(MT.AuthResponse,
        JSON.stringify({ status: 'error', error: 'Username reserved' }));
    }

    const result = db.registerUser(json.username, json.password);
    if (result) {
      this.userId = result.id;
      this.username = result.username;
      this.authenticated = true;
      sessions.set(this.userId, this);
      if (json.publicKey) db.setPublicKey(this.userId, json.publicKey);

      this.send(MT.AuthResponse,
        JSON.stringify({ status: 'ok', user_id: result.id, username: result.username }));
      broadcastStatus(this.userId, this.username, true);

      // Welcome messages
      setTimeout(() => {
        for (const [gid, g] of Object.entries(db.data.groups)) {
          if (db.data.members[gid] && db.data.members[gid][result.id]) {
            sendSystemMessage(gid, `👋 Welcome to #${g.name}, ${result.username}!`);
          }
        }
      }, 500);
    } else {
      this.send(MT.AuthResponse,
        JSON.stringify({ status: 'error', error: 'Username taken' }));
    }
  }

  handlePubKeyRequest(json) {
    const pubkey = db.getPublicKey(json.user_id);
    this.send(MT.PubKeyResponse,
      JSON.stringify({ user_id: json.user_id, public_key: pubkey }));
  }

  handleTextMessage(json) {
    if (!this.authenticated) return;
    const target = sessions.get(json.recipient_id);
    if (!target) return;

    if (db.isBanned(this.userId)) {
      this.send(MT.Error, JSON.stringify({ error: 'You are banned from Nullchat' }));
      return;
    }

    target.send(MT.TextMessage, JSON.stringify({
      sender_id: this.userId, sender_name: this.username,
      content: json.content, encrypted: json.encrypted || false,
      timestamp: json.timestamp || Date.now()
    }));
  }

  handleGroupMessage(json) {
    if (!this.authenticated) return;

    if (db.isBanned(this.userId)) {
      this.send(MT.Error, JSON.stringify({ error: 'You are banned from Nullchat' }));
      return;
    }

    if (db.isGroupBanned(json.group_id, this.userId)) {
      this.send(MT.Error, JSON.stringify({ error: 'You are banned from this group' }));
      return;
    }

    // Check for admin commands
    if (this.isNull && json.content.startsWith('/')) {
      this.handleAdminCommand(json.group_id, json.content);
      return;
    }

    const members = db.getGroupMembers(json.group_id);
    const fwd = JSON.stringify({
      sender_id: this.userId, sender_name: this.username,
      content: json.content, encrypted: json.encrypted || false,
      timestamp: json.timestamp || Date.now(),
      group_id: json.group_id
    });
    for (const mid of members) {
      if (mid === this.userId) continue;
      const s = sessions.get(mid);
      if (s) {
        s.send(MT.GroupMessage, fwd);
        // Check for @mentions
        const mentionedUser = db.getUserById(mid);
        if (mentionedUser && json.content.includes('@' + mentionedUser.username)) {
          const g = db.data.groups[json.group_id];
          s.send(MT.Mention, JSON.stringify({
            sender_id: this.userId, sender_name: this.username,
            group_id: json.group_id, group_name: g ? g.name : '',
            content: json.content
          }));
        }
      }
    }
  }

  handleAdminCommand(groupId, cmd) {
    const parts = cmd.split(' ');
    const command = parts[0].toLowerCase();

    if (command === '/ban' && parts[1]) {
      const targetId = db.getUserIdByUsername(parts[1]);
      if (targetId) {
        db.banUser(targetId);
        db.removeMember(groupId, targetId);
        const s = sessions.get(targetId);
        if (s) {
          s.send(MT.Error, JSON.stringify({ error: 'You have been banned from Nullchat by TheNull' }));
          s.socket.end();
        }
        sendSystemMessage(groupId, `${parts[1]} has been banned from Nullchat by TheNull`);
      }
    } else if (command === '/unban' && parts[1]) {
      const targetId = db.getUserIdByUsername(parts[1]);
      if (targetId) {
        db.unbanUser(targetId);
        sendSystemMessage(groupId, `${parts[1]} has been unbanned by TheNull`);
      }
    } else if (command === '/gban' && parts[1] && parts[2]) {
      const targetId = db.getUserIdByUsername(parts[2]);
      const gid = db.getGeneralGroupId();
      const targetGroup = parts[1] === 'general' ? gid : null;
      if (targetId && targetGroup) {
        db.banFromGroup(targetGroup, targetId);
        db.removeMember(targetGroup, targetId);
        sendSystemMessage(groupId, `${parts[2]} has been banned from ${parts[1]} by TheNull`);
      }
    } else if (command === '/kick') {
      // Remove from general
      const gid = db.getGeneralGroupId();
      if (gid) db.removeMember(gid, this.userId);
      this.send(MT.Error, JSON.stringify({ error: 'You have been kicked from the server' }));
      this.socket.end();
    } else if (command === '/help') {
      sendSystemMessage(groupId,
        'commands: /ban <user> /unban <user> /gban <group> <user> /broadcast <msg>');
    } else if (command === '/broadcast') {
      const msg = parts.slice(1).join(' ');
      if (msg) {
        const gid = db.getGeneralGroupId();
        if (gid) sendSystemMessage(gid, `📢 ${msg}`);
      }
    }
  }

  handleCreateGroup(json) {
    if (!this.authenticated) return;
    if (db.isBanned(this.userId)) {
      return this.send(MT.Error, JSON.stringify({ error: 'You are banned' }));
    }
    const gid = db.createGroup(json.name, this.userId);
    this.send(MT.GroupInfo, JSON.stringify({ status: 'ok', group_id: gid, name: json.name }));
  }

  handleJoinGroup(json) {
    if (!this.authenticated) return;
    if (db.isGroupBanned(json.group_id, this.userId)) {
      return this.send(MT.Error, JSON.stringify({ error: 'You are banned from this group' }));
    }
    db.addMember(json.group_id, this.userId, json.encrypted_key || '');
    this.send(MT.GroupInfo, JSON.stringify({ status: 'ok', group_id: json.group_id }));
  }

  handleRenameGroup(json) {
    if (!this.authenticated || !this.isNull) return;
    if (db.data.groups[json.group_id]) {
      db.data.groups[json.group_id].name = json.name;
      db.save();
      sendSystemMessage(json.group_id, `channel renamed to #${json.name} by TheNull`);
    }
  }

  handleDeleteGroup(json) {
    if (!this.authenticated || !this.isNull) return;
    if (db.data.groups[json.group_id]) {
      delete db.data.groups[json.group_id];
      delete db.data.members[json.group_id];
      db.save();
    }
  }

  handleP2POffer(json) {
    if (!this.authenticated) return;
    const target = sessions.get(json.target_id);
    if (target) {
      target.send(MT.P2POffer, JSON.stringify({
        from_id: this.userId, from_username: this.username,
        sdp: json.sdp, public_key: json.public_key || ''
      }));
    }
  }

  handleP2PAnswer(json) {
    if (!this.authenticated) return;
    const target = sessions.get(json.target_id);
    if (target) {
      target.send(MT.P2PAnswer, JSON.stringify({
        from_id: this.userId, sdp: json.sdp, public_key: json.public_key || ''
      }));
    }
  }

  handleP2PICE(json) {
    if (!this.authenticated) return;
    const target = sessions.get(json.target_id);
    if (target) {
      target.send(MT.P2PICECandidate, JSON.stringify({
        from_id: this.userId, candidate: json.candidate
      }));
    }
  }
}

function broadcastStatus(userId, username, online) {
  const pubkey = db.getPublicKey(userId);
  const msg = JSON.stringify({ user_id: userId, username, online, public_key: pubkey });
  for (const [sid, s] of sessions) {
    if (sid !== userId) s.send(online ? MT.UserOnline : MT.UserOffline, msg);
  }
}

// ── Start ─────────────────────────────────────────────────────────────────
const server = net.createServer((socket) => new Session(socket));

server.listen(PORT, '0.0.0.0', () => {
  console.log(`Nullchat server listening on port ${PORT}`);
  console.log(`DB: ${DB_PATH}`);
  const gid = db.getGeneralGroupId();
  console.log(`#general ID: ${gid}`);
});

process.on('SIGINT', () => { console.log('\nShutting down...'); process.exit(0); });
process.on('SIGTERM', () => process.exit(0));
