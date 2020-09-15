<template>
  <div id="app" class="page-container">
    <link rel="stylesheet" href="//fonts.googleapis.com/css?family=Roboto:400,500,700,400italic|Material+Icons">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css">

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
          </md-list-item>

          <md-list-item to="/display" @click="clickSidebar">
            <md-icon>queue_play_next</md-icon>
            <span class="md-list-item-text">Display</span>
          </md-list-item>

          <md-list-item to="/images" @click="clickSidebar">
            <md-icon>wallpaper</md-icon>
            <span class="md-list-item-text">Images</span>
          </md-list-item>

          <md-list-item to="/controls" @click="clickSidebar">
            <md-icon>settings_remote</md-icon>
            <span class="md-list-item-text">Controls</span>
          </md-list-item>

          <md-list-item to="/update" @click="clickSidebar">
            <md-icon>system_update</md-icon>
            <span class="md-list-item-text">Update</span>
          </md-list-item>

          <md-list-item to="/editor" @click="clickSidebar">
            <md-icon>edit</md-icon>
            <span class="md-list-item-text">Raw Editor</span>
          </md-list-item>

          <md-list-item to="/settings" @click="clickSidebar">
            <md-icon>settings</md-icon>
            <span class="md-list-item-text">Settings</span>
          </md-list-item>
        </md-list>
      </md-app-drawer>

      <md-app-content>
        <vue-progress-bar></vue-progress-bar>
        <router-view class="router"></router-view>
      </md-app-content>
    </md-app>

  </div>
</template>

<script>
export default {
  name: 'App',
  mounted() {
    this.$Progress.finish()
    this.isMobile = this.$isMobile()
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

.md-app {
  height: 100vh;
}
</style>
