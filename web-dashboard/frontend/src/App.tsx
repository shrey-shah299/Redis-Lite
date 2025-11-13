import  { useState, useEffect } from 'react';
import TaskCreator from './components/TaskCreator';
import RedisStatus from './components/RedisStatus';
import WorkerPanel from './components/WorkerPanel';
import { api, DashboardStats } from './services/api';

function App() {
  const [connected, setConnected] = useState(false);
  const [stats, setStats] = useState<DashboardStats | null>(null);
  const [refreshTrigger, setRefreshTrigger] = useState(0);

  const checkConnection = async () => {
    try {
      const result = await api.checkRedisStatus();
      setConnected(result.connected);
    } catch {
      setConnected(false);
    }
  };

  const fetchStats = async () => {
    try {
      const data = await api.getDashboardStats();
      setStats(data);
    } catch (err) {
      console.error('Error fetching stats:', err);
    }
  };

  useEffect(() => {
    checkConnection();
    fetchStats();

    // Poll connection and stats every 5 seconds
    const interval = setInterval(() => {
      checkConnection();
      fetchStats();
    }, 5000);

    return () => clearInterval(interval);
  }, [refreshTrigger]);

  const handleTaskCreated = () => {
    // Trigger refresh when a task is created
    setRefreshTrigger(prev => prev + 1);
  };

  return (
    <div className="min-h-screen bg-gray-900 text-white">
      {/* Top Bar */}
      <div className="bg-gray-800 border-b border-gray-700 px-6 py-4">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div>
              <h1 className="text-2xl font-bold">Redis-Lite Dashboard</h1>
              <p className="text-sm text-gray-400">Task Queue Monitoring System</p>
            </div>
          </div>

          <div className="flex items-center gap-6">
          </div>
        </div>
      </div>

      {/* Main Content */}
      <div className="p-6">
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 h-[calc(100vh-140px)]">
          {/* Left Panel: Task Creator */}
          <div className="lg:col-span-1">
            <TaskCreator onTaskCreated={handleTaskCreated} />
          </div>

          {/* Middle Panel: Redis Status */}
          <div className="lg:col-span-1">
            <RedisStatus key={refreshTrigger} />
          </div>

          {/* Right Panel: Worker Status */}
          <div className="lg:col-span-1">
            <WorkerPanel />
          </div>
        </div>
      </div>

      {/* Footer */}
      <div className="fixed bottom-0 left-0 right-0 bg-gray-800 border-t border-gray-700 px-6 py-2 text-center text-xs text-gray-500">
        Redis-Lite Task Queue Dashboard | Workers auto-refresh: 2s | Stats auto-refresh: 5s
      </div>
    </div>
  );
}

export default App;
