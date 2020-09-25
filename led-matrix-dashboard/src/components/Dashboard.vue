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
          <div class="md-list-item stat-value"> {{ prettyPrintResetReason }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Firmware Version </b>
          <div class="md-list-item stat-value"> v{{ this.$store.state.stats['version'] }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Dashboard Version </b>
          <div v-bind:style="{color: $store.state.stats['version'] === this.$dashboardVersion ? '#000000' : '#ff0000'}" class="md-list-item stat-value"> v{{ $dashboardVersion }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Fragmentation </b>
          <div v-bind:style="{color: this.$store.state.stats['fragmentation'] < 50 ? '#000000' : '#ff0000'}" class="md-list-item stat-value"> {{ this.$store.state.stats['fragmentation'] }}%</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Free Memory </b>
          <div v-bind:style="{color: this.$store.state.stats['memoryFree'] > 10000 ? '#000000' : '#ff0000'}" class="md-list-item stat-value"> {{ this.$store.state.stats['memoryFree'] }} Bytes</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Transparency Buffer </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['transparencyBuffer'] ? 'Allocated' : 'Unallocated' }}</div>
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
          <b class="md-list-item stat-name"> Custom Image Limit </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['maxOpenFiles'] - 1 }} Images</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Max Image Name Length</b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['maxPathLength'] - 8 }}</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Display Width </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['width'] }} Pixels</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Display Height </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['height'] }} Pixels</div>
        </md-list-item>
        <md-list-item class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Display Brightness </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['brightness'] }}%</div>
        </md-list-item>
        <md-list-item v-if="this.$store.state.stats['brightnessSensor'] !== 65535" class="md-layout md-gutter md-alignment-center-center">
          <b class="md-list-item stat-name"> Brightness Sensor </b>
          <div class="md-list-item stat-value"> {{ this.$store.state.stats['brightnessSensor'] }}</div>
        </md-list-item>
        <md-list-item v-if="this.$store.state.stats['vcc'] !== 65535" class="md-layout md-gutter md-alignment-center-center">
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
  computed: {
    prettyPrintResetReason: function () {
      let reason = this.$store.state.stats['resetReason']
      switch (reason) {
        case 'Hardware Watchdog':
        case 'Exception':
        case 'Software Watchdog':
        case 'Unknown':
          return reason
        default:
          return 'Normal'
      }
    }
  },
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
