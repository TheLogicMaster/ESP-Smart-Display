<template>
  <div>
    <md-card>
      <h1>Dashboard</h1>
      <md-list>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Uptime </b>
          <div class="md-list-item stat-value"> {{ [this.$store.state.stats['uptime'], 'seconds'] | duration('humanize', false) }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Reset Reason </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['resetReason'] }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Firmware Version </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['version'] }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Dashboard Version </b>
          <div class="md-list-item stat-value"> {{ $dashboardVersion }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Fragmentation </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['fragmentation'] }}%</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Free Memory </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['memoryFree'] }} Bytes</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Filesystem Usage </b>
          <div class="md-list-item stat-value"> {{ (this.$store.state.stats['filesystemUsed'] / 1000).toFixed(1) }} KB ({{ (this.$store.state.stats['filesystemUsed'] / this.$store.state.stats['filesystemTotal'] * 100).toFixed(2) }}%) </div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Filesystem Size</b>
          <div class="md-list-item stat-value"> {{ (this.$store.state.stats['filesystemTotal'] / 1000000).toFixed(2) }} MB</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Width </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['width'] }} Pixels</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Height </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['height'] }} Pixels</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Power Supply Voltage </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['vcc'] / 1000 }}V</div>
        </md-list-item>
      </md-list>
      <md-button class="md-raised md-accent" @click="getStats">Refresh</md-button>
      <md-checkbox class="md-layout-item md-accent" v-model="autoRefresh" @change="changeAutoRefresh">Auto-Refresh</md-checkbox>
    </md-card>
  </div>
</template>

<script>
export default {
  name: "Dashboard",
  data: () => ({
    stats: {},
    autoRefresh: false,
    timer: null
  }),
  mounted() {
    this.getStats()
  },
  methods: {
    changeAutoRefresh() {
      if (this.autoRefresh)
        this.timer = setInterval(this.getStats, 5000)
      else
        clearInterval(this.timer)
    }
  },
  beforeDestroy() {
    clearInterval(this.timer)
  }
}
</script>

<style scoped>
.md-card {
  display: inline-block;
}

.stat-name {
  margin-right: 70px;
  margin-left: 20px;
}

.stat-value {
  text-align: right;
  margin-right: 20px;
  margin-left: 20px;
}
</style>
