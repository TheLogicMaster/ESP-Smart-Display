<template>
  <div>
    <md-card>
      <h1>Dashboard</h1>
      <md-list>
        <md-list-item class="md-layout md-gutter md-alignment-center-center"
                      v-for="(stat, name) in this.$store.state.stats">
          <b class="md-list-item stat-name"> {{ name }} </b>
          <div class="md-list-item stat-value"> {{ stat }}</div>
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
        this.timer = setInterval(this.update, 5000);
      else
        clearInterval(this.timer);
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
