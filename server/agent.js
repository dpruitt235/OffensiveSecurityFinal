const crypto = require('crypto');

module.exports = class Agent {
  constructor({hostname, user, os, arch, sdk}) {
    this.hostname = hostname;
    this.user = user;
    this.os = os;
    this.arch = arch;
    this.sdk = sdk;

    this.lastContacted = new Date();
    this.commands = []
    this.id = (
      crypto.createHash('sha1').update(hostname + user + os).digest('hex').slice(0, 5)
    );
  }

  queueCommand(command) {
    this.commands.push(command);
  }

  getCommand() {
    return this.commands.shift();
  }

  toJSON() {
    return {
      hostname: this.hostname,
      user: this.user,
      os: this.os,
      id: this.id,
      lastContacted: this.lastContacted,
    }
  }
}
