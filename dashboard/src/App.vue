<template>
  <div id="app" class="page-container">
    <link rel="stylesheet" href="//fonts.googleapis.com/css?family=Roboto:400,500,700,400italic|Material+Icons">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css">

    <div class="backdrop" v-if="$store.state.backdrop"></div>

    <vue-snotify></vue-snotify>

    <md-app>
      <md-app-toolbar class="md-primary" md-elevation="0">
        <md-button class="md-icon-button" @click="toggleSidebar" v-if="!sidebar">
          <md-icon>menu</md-icon>
        </md-button>
        <span class="md-title">ESP LED Matrix Display</span>
      </md-app-toolbar>

      <md-app-drawer :md-active.sync="sidebar" :md-persistent="isMobile ? null : 'mini'">
        <md-toolbar class="md-transparent" md-elevation="0">
          <h1>LED Matrix Display</h1>

          <div class="md-toolbar-section-end">
            <md-button class="md-icon-button md-dense" @click="toggleSidebar">
              <md-icon>keyboard_arrow_left</md-icon>
            </md-button>
          </div>
        </md-toolbar>

        <md-list>
          <md-list-item to="/" @click="clickSidebar">
            <md-icon>dashboard</md-icon>
            <span class="md-list-item-text">Dashboard</span>
            <md-tooltip md-delay="1000" md-direction="right"> View display status </md-tooltip>
          </md-list-item>

          <md-list-item to="/display" @click="clickSidebar">
            <md-icon>queue_play_next</md-icon>
            <span class="md-list-item-text">Display</span>
            <md-tooltip md-delay="1000" md-direction="right"> Edit display widgets </md-tooltip>
          </md-list-item>

          <md-list-item to="/images" @click="clickSidebar">
            <md-icon>wallpaper</md-icon>
            <span class="md-list-item-text">Images</span>
            <md-tooltip md-delay="1000" md-direction="right"> Manage display images </md-tooltip>
          </md-list-item>

          <md-list-item to="/controls" @click="clickSidebar">
            <md-icon>settings_remote</md-icon>
            <span class="md-list-item-text">Controls</span>
            <md-tooltip md-delay="1000" md-direction="right"> Send commands to display </md-tooltip>
          </md-list-item>

          <md-list-item to="/update" @click="clickSidebar">
            <md-icon>system_update</md-icon>
            <span class="md-list-item-text">Update</span>
            <md-tooltip md-delay="1000" md-direction="right"> Update the display </md-tooltip>
          </md-list-item>

          <md-list-item to="/editor" @click="clickSidebar">
            <md-icon>edit</md-icon>
            <span class="md-list-item-text">Raw Editor</span>
            <md-tooltip md-delay="1000" md-direction="right"> Edit the raw configuration file </md-tooltip>
          </md-list-item>

          <md-list-item to="/settings" @click="clickSidebar">
            <md-icon>settings</md-icon>
            <span class="md-list-item-text">Settings</span>
            <md-tooltip md-delay="1000" md-direction="right"> Configure display settings </md-tooltip>
          </md-list-item>

          <md-list-item to="/backup" @click="clickSidebar">
            <md-icon>archive</md-icon>
            <span class="md-list-item-text">Backup</span>
            <md-tooltip md-delay="1000" md-direction="right"> Backup and restore display configuration </md-tooltip>
          </md-list-item>
        </md-list>
      </md-app-drawer>

      <md-app-content>
        <vue-progress-bar></vue-progress-bar>
        <router-view v-if="$store.state.stats && $store.state.configuration" class="router"></router-view>
      </md-app-content>
    </md-app>

  </div>
</template>

<script>
export default {
  name: 'App',
  async mounted() {
    this.$Progress.finish()
    this.isMobile = this.$isMobile()
    await this.waitForPromiseSuccess(this.getStats)
    await this.waitForPromiseSuccess(this.getConfig)
    if (await this.checkForUpdate())
      this.$snotify.info('A new version of the firmware is available', 'Update Available', {
        closeOnClick: true,
        timeout: 0
      })
  },
  data: () => ({
    sidebar: false,
    isMobile: false
  }),
  methods: {
    clickSidebar() {
      if (this.isMobile)
        this.sidebar = false
    },
    toggleSidebar() {
      if (this.isMobile)
        this.sidebar = true
      else
        this.sidebar = !this.sidebar;
    }
  },
  created() {
    this.$Progress.start()
    this.$router.beforeEach((to, from, next) => {
      if (to.meta.progress !== undefined) {
        let meta = to.meta.progress
        this.$Progress.parseMeta(meta)
      }
      this.$Progress.start()
      next()
    })
    this.$router.afterEach((to, from) => {
      this.$Progress.finish()
    })
  }
}
</script>

<style>
@import "~vue-snotify/styles/material.css";

#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
//margin-top: 60px;
}

canvas {
  image-rendering: -moz-crisp-edges;
  image-rendering: -webkit-crisp-edges;
  //image-rendering: pixelated;
  image-rendering: crisp-edges;
}

img {
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}

.inline-centered {
  display: inline-block;
  vertical-align: middle;
}

.wide {
  width: 100%;
}

.backdrop {
  position: fixed;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  z-index: 500;
}

.md-app {
  height: 100vh;
}

.bounceInDown {
  animation-name: bounceInDown;
}

@keyframes bounceInDown {
  0%, 60%, 75%, 90%, 100% {
    transition-timing-function: cubic-bezier(0.215, 0.610, 0.355, 1.000);
  }

  0% {
    opacity: 0;
    transform: translate3d(0, -3000px, 0) scaleY(5);
  }

  60% {
    opacity: 1;
    transform: translate3d(0, 25px, 0) scaleY(.9);
  }

  75% {
    transform: translate3d(0, -10px, 0) scaleY(.95);
  }

  90% {
    transform: translate3d(0, 5px, 0) scaleY(.985);
  }

  100% {
    opacity: 1;
    transform: none;
  }
}
</style>
