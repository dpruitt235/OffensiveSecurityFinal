const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
});

function start(prompt, eventEmitter) {
  rl.question(prompt, (answer) => {
    eventEmitter.emit(...answer.split(' '));
    start(prompt, eventEmitter);
  });
}

module.exports = {
  start,
};
