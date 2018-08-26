'use strict'

const { remote } = require('electron')

module.exports = function (name) {
  if (remote) {
    return remote.require(name)
  } else {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
}
